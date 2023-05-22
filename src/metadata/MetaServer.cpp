#include <cassert>
#include <cstddef>
#include <cstdint>
#include <grpcpp/server_context.h>
#include <grpcpp/support/status.h>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <map>
#include <iterator>
#include <random>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <sys/types.h>
#include <Configuration.hpp>
#include "file.hpp"
#include "spacemanager.hpp"
#include "spongebob.grpc.pb.h"
#include "spongebob.pb.h"
#include "debug.hpp"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using namespace spongebob;

#define ROOT_INUM (0)

inline int randInt(int n) {
  return rand() % n;
}

// Logic and data behind the server's behavior.
class MetadataServiceImpl final : public Metadata::Service {
public:
  MetadataServiceImpl(int _server_count = 1) : server_count(_server_count) {
    inode_table_ = std::make_shared<InodeTable>(100);
    file_map_ = std::make_shared<FileMap>(100);
    //ligch: just for fuse tests
    // space_manager_[1] = std::make_shared<SpaceManager>(0, 1 << 30, FILE_BLOCK_SIZE, 1);
    // space_manager_[2] = std::make_shared<SpaceManager>(0, 1 << 20, FILE_BLOCK_SIZE, 2);
    // space_manager_ = std::make_shared<SpaceManager>(0, 1 << 30, FILE_BLOCK_SIZE, server_count);
  }

  Status SayHello(ServerContext* context, const HelloRequest* request,
                  HelloReply* reply) override {
    std::string prefix("Hello ");
    reply->set_message(prefix + request->name());
    return Status::OK;
  }

  void InitBlockInfo(spongebob::FileBlockInfo* block_info,
    uint64_t block_idx, uint64_t server_id, uint64_t mem_offset, uint64_t length, uint64_t buff_offset) {
    block_info->set_block_idx(block_idx);
    block_info->set_serverid(server_id);
    block_info->set_mem_offset(mem_offset);
    block_info->set_length(length);
    block_info->set_buff_offset(buff_offset);
  }

  Status ReadFile(ServerContext* context, const ReadRequest* request,
                  ReadReply* reply) override {
    /* Suppose only one directory and its inum is 0. */
    /* Path parsing have not implemented yet. */
    auto root_inode = inode_table_->GetInode(ROOT_INUM);
    if (root_inode == nullptr) {
      std::cerr << __func__ << ": root dir's inode doesn't exist." << std::endl;
      return Status::CANCELLED;
    }
    std::string file_name = request->name();
    uint64_t start_offset = request->offset();
    uint64_t read_length = request->length();
    auto file_dentry = root_inode->GetDentry(file_name);
    uint64_t file_inum = file_dentry->GetInodeNum();

    if (file_dentry == nullptr) {
      std::cerr << __func__ << ": file " << request->name() << " doesn't exist." << std::endl;
      return Status::CANCELLED;
    }

    auto file_inode = inode_table_->GetInode(file_inum);
    assert(file_inode != nullptr);

    if (start_offset + read_length > file_inode->GetSize()) {
      uint64_t old_read_length = read_length;
      read_length = file_inode->GetSize() - start_offset;
      Debug::debugItem("%s: expected read size out of range.\n", __func__);
      Debug::debugItem("%s: read length change from %lu to %lu\n", __func__, old_read_length, read_length);
      // return Status::CANCELLED;
    }

    uint64_t to_read_length = read_length;
    uint64_t buff_offset = 0;

    uint64_t start_block = start_offset / FILE_BLOCK_SIZE;

    /* First unaligned block. */
    if (start_offset & FILE_BLOCK_MASK) {
      auto cur_block_info = file_inode->GetBlockInfo(start_block);
      auto first_block_info = reply->add_block_info();
      auto inner_offset = start_offset & FILE_BLOCK_MASK;
      auto bytes_read = std::min(FILE_BLOCK_SIZE - inner_offset, to_read_length);
      InitBlockInfo(first_block_info, start_block, cur_block_info.server_id, cur_block_info.start_addr + inner_offset, bytes_read, buff_offset);
      buff_offset += bytes_read;
      to_read_length -= bytes_read;
      start_block++;
    }

    while (to_read_length >= FILE_BLOCK_SIZE) {
      auto cur_block_info = file_inode->GetBlockInfo(start_block);
      auto block_info = reply->add_block_info();
      InitBlockInfo(block_info, start_block, cur_block_info.server_id, cur_block_info.start_addr, FILE_BLOCK_SIZE, buff_offset);
      buff_offset += FILE_BLOCK_SIZE;
      to_read_length -= FILE_BLOCK_SIZE;
      start_block++;
    }

    if (to_read_length) {
      auto cur_block_info = file_inode->GetBlockInfo(start_block);
      auto last_block_info = reply->add_block_info();
      InitBlockInfo(last_block_info, start_block, cur_block_info.server_id, cur_block_info.start_addr, to_read_length, buff_offset);
      buff_offset += FILE_BLOCK_SIZE;
      to_read_length = 0;
    }
    reply->set_bytes_read(read_length - to_read_length);
    return Status::OK;
  }

  Status WriteFile(ServerContext* context, const WriteRequest* request,
                   WriteReply* reply) override {
    auto root_inode = inode_table_->GetInode(ROOT_INUM);
    if (root_inode == nullptr) {
      std::cerr << __func__ << ": root dir's inode doesn't exist." << std::endl;
      return Status::CANCELLED;
    }
    std::string file_name = request->name();

    auto file_dentry = root_inode->GetDentry(file_name);

    /* File doen't exist, create it. */
    if (file_dentry == nullptr) {
      Debug::notifyInfo("%s: file %s not exists. Create...\n", __func__, file_name.c_str());
      CreateFile_(file_name, false);
      file_dentry = root_inode->GetDentry(file_name);
    }

    uint64_t file_inum = file_dentry->GetInodeNum();
    auto file_inode = inode_table_->GetInode(file_inum);
    uint64_t start_offset = request->offset();
    uint64_t write_length = request->length();
    uint64_t to_write_length = write_length;
    uint64_t buff_offset = 0;
    uint64_t total_blocks = (start_offset + write_length + FILE_BLOCK_SIZE - 1) / FILE_BLOCK_SIZE;
    uint64_t start_block = start_offset / FILE_BLOCK_SIZE;

    /* Enlarge file. Allocate new blocks. */
    if (file_inode->GetBlockNum() < total_blocks) {
      int new_blocks_num = total_blocks - file_inode->GetBlockNum();
      // todo: change server id, now suppose server id = 0;
      for (int i = 0; i < new_blocks_num; ++i) {
        // auto server_id = randInt(server_count);
        // if (server_id == 0)
        //   server_id = server_count;
        // std::cout << "Rand choose server " << server_id << std::endl;
        auto r = randInt(space_manager_.size());
        int curr_idx = 0, server_id = -1;
        for (auto iter = space_manager_.begin(); iter != space_manager_.end(); ++iter, ++curr_idx) {
          if (curr_idx == r) {
            server_id = iter->first;
            break;
          }
        }
        uint64_t block_nr = space_manager_[server_id]->AllocateOneBlock();
        file_inode->AppendFileBlockInfo(server_id, block_nr * FILE_BLOCK_SIZE, 0, block_nr);
      }
      // std::cout << new_blocks_num << " blocks allocated to file " << file_name << std::endl;
    }

    if (start_offset & FILE_BLOCK_MASK) {
      auto cur_block_info = file_inode->GetBlockInfo(start_block);
      auto cur_length = cur_block_info.length;
      auto first_block_info = reply->add_block_info();
      auto inner_offset = start_offset & FILE_BLOCK_MASK;
      auto bytes_write = std::min(FILE_BLOCK_SIZE - inner_offset, to_write_length);

      InitBlockInfo(first_block_info, start_block, cur_block_info.server_id, cur_block_info.start_addr + inner_offset, bytes_write, buff_offset);
      buff_offset += bytes_write;
      to_write_length -= bytes_write;
      file_inode->ChangeFileBlockInfoLength(start_block, cur_length + bytes_write);
      start_block++;
    }

    while (to_write_length >= FILE_BLOCK_SIZE) {
      auto cur_block_info = file_inode->GetBlockInfo(start_block);
      auto block_info = reply->add_block_info();
      InitBlockInfo(block_info, start_block, cur_block_info.server_id, cur_block_info.start_addr, FILE_BLOCK_SIZE, buff_offset);
      buff_offset += FILE_BLOCK_SIZE;
      to_write_length -= FILE_BLOCK_SIZE;
      file_inode->ChangeFileBlockInfoLength(start_block, FILE_BLOCK_SIZE);
      start_block++;
    }

    if (to_write_length) {
      auto cur_block_info = file_inode->GetBlockInfo(start_block);
      auto last_block_info = reply->add_block_info();
      InitBlockInfo(last_block_info, start_block, cur_block_info.server_id, cur_block_info.start_addr, to_write_length, buff_offset);
      file_inode->ChangeFileBlockInfoLength(start_block, to_write_length);
      to_write_length = 0;
    }

    file_inode->SetSize(start_offset + write_length - to_write_length);
    reply->set_bytes_write(write_length - to_write_length);
    return Status::OK;
  }

  Status CreateFile(ServerContext* context, const CreateRequest* request,
                  CreateReply* reply) override{
    auto root_inode = inode_table_->GetInode(ROOT_INUM);
    if (root_inode == nullptr) {
      std::cerr << __func__ << ": root dir's inode doesn't exist." << std::endl;
      return Status::CANCELLED;
    }

    auto filename = request->name();
    std::shared_ptr<Dentry> dentry;
    if ((dentry = root_inode->GetDentry(filename)) != nullptr) {
      uint64_t inum = dentry->GetInodeNum();
      Debug::notifyInfo("%s: file %s already exist, inum %lu", __func__, filename.c_str(), inum);
      reply->set_inum(dentry->GetInodeNum());
      return Status::OK;
    }
    uint64_t new_inum;
    if (!request->is_dir()) {
      new_inum = inode_table_->AllocateFileInode();
    } else {
      new_inum = inode_table_->AllocateDirInode();
    }

    Debug::debugItem("%s: alloc inode %lu to file %s\n", __func__, new_inum, filename.c_str());
    Debug::debugItem("%s: inode table current size: %lu\n", __func__, inode_table_->GetCurSize());
    root_inode->AddDentry(filename, new_inum);
    reply->set_inum(new_inum);

    return Status::OK;
  }

  Status RegisterMemoryRegion(ServerContext* context, const RegisterMemoryRegionRequest* request,
                  RegisterMemoryRegionReply* reply) override {
    Debug::notifyInfo("RegisterMemoryRegion: %d 0x%lx Size: %lu", request->nodeid(), request->addr(), request->length());
    uint64_t start_addr = request->addr();
    uint64_t length = request->length();
    space_manager_[request->nodeid()] = std::make_shared<SpaceManager>(start_addr, start_addr + length - 1, FILE_BLOCK_SIZE, request->nodeid());
    // std::cout << "Hello here\n";
    // space_manager_->ResetSpaceRange(start_addr, length);
    // space_manager_ = std::make_shared<SpaceManager>(start_addr, length, FILE_BLOCK_SIZE);
    return Status::OK;
  }

  Status ReadDirectory(ServerContext* context, const ReadDirectoryRequest* request,
                  ReadDirectoryReply* reply) override {
    /* The file system only has root directory now. */
    std::string path = request->path();
    auto root_inode = inode_table_->GetInode(ROOT_INUM);
    if (root_inode == nullptr) {
      std::cerr << __func__ << ": root dir's inode doesn't exist." << std::endl;
      return Status::CANCELLED;
    }
    auto dentry_map = root_inode->GetDentryMap();
    auto root_dentry = reply->add_dentry_info();
    root_dentry->set_name(".");
    root_dentry->set_inum(ROOT_INUM);
    root_dentry->set_is_dir(true);
    root_dentry->set_size(FILE_BLOCK_SIZE);

    auto parent_dentry = reply->add_dentry_info();
    parent_dentry->set_name("..");
    parent_dentry->set_inum(ROOT_INUM);
    parent_dentry->set_is_dir(true);
    parent_dentry->set_size(FILE_BLOCK_SIZE);

    for (auto it = dentry_map.begin(); it != dentry_map.end(); ++it) {
      auto dentry_info = reply->add_dentry_info();
      auto file_inode = inode_table_->GetInode(it->second->GetInodeNum());
      // std::cout << it->second->GetName() << " " << it->second->GetInodeNum() << std::endl;
      dentry_info->set_name(it->second->GetName());
      dentry_info->set_inum(it->second->GetInodeNum());
      dentry_info->set_is_dir(file_inode->IsDir());
      dentry_info->set_size(file_inode->GetSize());
    }
    return Status::OK;
}

  Status OpenFile(ServerContext* context, const OpenRequest* request,
                  OpenReply* reply) override{
    auto root_inode = inode_table_->GetInode(ROOT_INUM);

    if (root_inode == nullptr) {
      std::cerr << __func__ << ": root dir's inode doesn't exist." << std::endl;
      return Status::CANCELLED;
    }

    auto filename = request->name();
    Debug::notifyInfo("%s: start to open file %s", __func__, filename.c_str());
    std::shared_ptr<Dentry> dentry;
    if ((dentry = root_inode->GetDentry(filename)) == nullptr) {
      Debug::notifyInfo("%s: file %s doesn't exist, start to create.", __func__, filename.c_str());
      CreateFile_(filename, false);
    }

    auto new_fd = file_map_->AllocateFD();
    Debug::notifyInfo("%s: alloc fd %lu", __func__, new_fd);
    reply->set_fd(new_fd);
    return Status::OK;
  }

  Status CloseFile(ServerContext* context, const CloseRequest* request,
                  CloseReply* reply) override{
    int64_t fd = request->fd();
    bool success = file_map_->ReclaimFD(fd);
    reply->set_success(success);
    Debug::notifyInfo("%s: close file, fd is %lu", __func__, fd);
    return Status::OK;
  }

  Status RemoveFile(ServerContext* context, const RemoveRequest* request,
                    RemoveReply* reply) override{

    auto root_inode = inode_table_->GetInode(ROOT_INUM);
    if (root_inode == nullptr) {
      std::cerr << __func__ << ": root dir's inode doesn't exist." << std::endl;
      reply->set_success(false);
      return Status::CANCELLED;
    }

    auto filename = request->name();
    std::shared_ptr<Dentry> dentry;
    if ((dentry = root_inode->GetDentry(filename)) == nullptr) {
      Debug::notifyInfo("%s: file %s doesn't exist.", __func__, filename.c_str());
      reply->set_success(false);
      return Status::OK;
    }

    uint64_t inum = dentry->GetInodeNum();
    root_inode->DeleteDentry(filename);
    for (auto &sm : space_manager_)
      sm.second->ReclaimInodeSpace(inode_table_->GetInode(inum));
    inode_table_->DeleteInode(inum);
    Debug::notifyInfo("%s: file %s is removed. Inode %lu is reclaimed.", __func__, filename.c_str(), inum);
    reply->set_success(true);
    return Status::OK;
  }

private:
  uint64_t CreateFile_(const std::string &name, bool is_dir) {
    auto root_inode = inode_table_->GetInode(ROOT_INUM);
    if (root_inode == nullptr) {
      std::cerr << __func__ << ": root dir's inode doesn't exist." << std::endl;
      return inode_table_->GetTotalSize() + 1 ;
    }
    auto new_inum = inode_table_->AllocateFileInode();
    root_inode->AddDentry(name, new_inum);
    return new_inum;
  }

  int server_count;
  std::shared_ptr<InodeTable> inode_table_;
  std::map<int, std::shared_ptr<SpaceManager>> space_manager_;
  std::shared_ptr<FileMap> file_map_;
  uint64_t cur_server_id_ = 1;
};

void RunServer(uint16_t port) {
  auto conf = new Configuration();
  auto confPort = stoi(conf->metaip.substr(conf->metaip.find(':')+1, conf->metaip.size()));
  std::string server_address = absl::StrFormat("0.0.0.0:%d", confPort);
  MetadataServiceImpl service(conf->getServerCount());

  grpc::EnableDefaultHealthCheckService(true);
  grpc::reflection::InitProtoReflectionServerBuilderPlugin();
  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  Debug::notifyInfo("Server listening on %s", server_address.c_str());

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}
