#include <cassert>
#include <cstddef>
#include <cstdint>
#include <grpcpp/server_context.h>
#include <grpcpp/support/status.h>
#include <iostream>
#include <memory>
#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include "common.hpp"
#include "file.hpp"
#include "spacemanager.hpp"
#include "spongebob.grpc.pb.h"
#include "spongebob.pb.h"
// #include
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using spongebob::Greeter;
using spongebob::HelloReply;
using spongebob::HelloRequest;
using spongebob::ReadRequest;
using spongebob::ReadReply;
using spongebob::WriteRequest;
using spongebob::WriteReply;
using spongebob::ListDirectoryRequest;
using spongebob::ListDirectoryReply;
using spongebob::CreateRequest;
using spongebob::CreateReply;

#define FILE_BLOCK_SIZE (1 << 12)
#define FILE_BLOCK_MASK (FILE_BLOCK_SIZE - 1)

// Logic and data behind the server's behavior.
class GreeterServiceImpl final : public Greeter::Service {
public:
  GreeterServiceImpl() {
    inode_table_ = std::make_shared<InodeTable>(100);
    space_manager_ = std::make_shared<SpaceManager>(0, 1 << 20, FILE_BLOCK_SIZE);
  }

  Status SayHello(ServerContext* context, const HelloRequest* request,
                  HelloReply* reply) override {
    std::string prefix("Hello ");
    reply->set_message(prefix + request->name());
    return Status::OK;
  }

  void InitBlockInfo(spongebob::FileBlockInfo* block_info, uint64_t server_id, uint64_t offset, uint64_t length) {
    block_info->set_serverid(server_id);
    block_info->set_offset(offset);
    block_info->set_length(length);
  }

  Status ReadFile(ServerContext* context, const ReadRequest* request,
                  ReadReply* reply) override {
    /* Suppose only one directory and its inum is 0. */
    /* Path parsing have not implemented yet. */
    auto root_inode = inode_table_->GetInode(0);
    if (root_inode == nullptr) {
      std::cerr << __func__ << ": root dir's inode doesn't exist." << std::endl;
      return Status::CANCELLED;
    }
    std::string file_name = request->name();
    uint64_t start_offset = request->offset();
    uint64_t read_length = request->length();
    uint64_t to_read_length = read_length;
    auto file_dentry = root_inode->GetDentry(file_name);

    if (file_dentry == nullptr) {
      std::cerr << __func__ << ": file " << request->name() << " doesn't exist." << std::endl;
      return Status::CANCELLED;
    }

    uint64_t file_inum = file_dentry->GetInodeNum();
    auto file_inode = inode_table_->GetInode(file_inum);
    assert(file_inode != nullptr);

    if (start_offset + read_length - 1 > file_inode->GetSize()) {
      std::cerr << __func__ << ": read out of range." << std::endl;
      return Status::CANCELLED;
    }

    uint64_t start_block = start_offset / FILE_BLOCK_SIZE;

    /* First unaligned block. */
    if (start_offset & FILE_BLOCK_MASK) {
      auto cur_block_info = file_inode->GetBlockInfo(start_block);
      auto first_block_info = reply->add_block_info();
      auto inner_offset = start_offset & FILE_BLOCK_MASK;
      auto bytes_read = std::min(FILE_BLOCK_SIZE - inner_offset, to_read_length);
      InitBlockInfo(first_block_info, cur_block_info.server_id, cur_block_info.start_addr + inner_offset, bytes_read);
      to_read_length -= bytes_read;
      start_block++;
    }

    while (to_read_length >= FILE_BLOCK_SIZE) {
      auto cur_block_info = file_inode->GetBlockInfo(start_block);
      auto block_info = reply->add_block_info();
      InitBlockInfo(block_info, cur_block_info.server_id, cur_block_info.start_addr, FILE_BLOCK_SIZE);
      to_read_length -= FILE_BLOCK_SIZE;
      start_block++;
    }

    if (to_read_length) {
      auto cur_block_info = file_inode->GetBlockInfo(start_block);
      auto last_block_info = reply->add_block_info();
      InitBlockInfo(last_block_info, cur_block_info.server_id, cur_block_info.start_addr, to_read_length);
      to_read_length = 0;
    }


    // for (int i = 0; i < 3; ++i) {
    //   auto cur_block = reply->add_block_info();
    //   cur_block->set_serverid(i);
    //   cur_block->set_offset(i * 2);
    //   cur_block->set_length(i * 3);
    // }
    return Status::OK;
  }

  Status WriteFile(ServerContext* context, const WriteRequest* request,
                   WriteReply* reply) override {
    auto root_inode = inode_table_->GetInode(0);
    if (root_inode == nullptr) {
      std::cerr << __func__ << ": root dir's inode doesn't exist." << std::endl;
      return Status::CANCELLED;
    }
    std::string file_name = request->name();

    auto file_dentry = root_inode->GetDentry(file_name);

    /* File doen't exist, create it. */
    if (file_dentry == nullptr) {
      CreateFile_(file_name, false);
      file_dentry = root_inode->GetDentry(file_name);
    }

    uint64_t file_inum = file_dentry->GetInodeNum();
    auto file_inode = inode_table_->GetInode(file_inum);
    uint64_t start_offset = request->offset();
    uint64_t write_length = request->length();
    uint64_t to_write_length = write_length;
    uint64_t total_blocks = (start_offset + write_length + FILE_BLOCK_SIZE - 1) / FILE_BLOCK_SIZE;
    uint64_t start_block = start_offset / FILE_BLOCK_SIZE;

    /* Enlarge file. Allocate new blocks. */
    if (file_inode->GetBlockNum() < total_blocks) {
      int new_blocks_num = total_blocks - file_inode->GetBlockNum();
      // todo: change server id, now suppose server id = 0;
      for (int i = 0; i < new_blocks_num; ++i) {
        uint64_t block_nr = space_manager_->AllocateOneBlock();
        file_inode->AppendFileBlockInfo(0, block_nr * FILE_BLOCK_SIZE, 0);
      }
      std::cout << new_blocks_num << " blocks allocated to file " << file_name << std::endl;
    }

    if (start_offset & FILE_BLOCK_MASK) {
      auto cur_block_info = file_inode->GetBlockInfo(start_block);
      auto cur_length = cur_block_info.length;
      auto first_block_info = reply->add_block_info();
      auto inner_offset = start_offset & FILE_BLOCK_MASK;
      auto bytes_write = std::min(FILE_BLOCK_SIZE - inner_offset, to_write_length);

      InitBlockInfo(first_block_info, cur_block_info.server_id, cur_block_info.start_addr + inner_offset, bytes_write);

      to_write_length -= bytes_write;
      file_inode->ChangeFileBlockInfoLength(start_block, cur_length + bytes_write);
      start_block++;
    }

    while (to_write_length >= FILE_BLOCK_SIZE) {
      auto cur_block_info = file_inode->GetBlockInfo(start_block);
      auto block_info = reply->add_block_info();
      InitBlockInfo(block_info, cur_block_info.server_id, cur_block_info.start_addr, FILE_BLOCK_SIZE);
      to_write_length -= FILE_BLOCK_SIZE;
      file_inode->ChangeFileBlockInfoLength(start_block, FILE_BLOCK_SIZE);
      start_block++;
    }

    if (to_write_length) {
      auto cur_block_info = file_inode->GetBlockInfo(start_block);
      auto last_block_info = reply->add_block_info();
      InitBlockInfo(last_block_info, cur_block_info.server_id, cur_block_info.start_addr, to_write_length);
      file_inode->ChangeFileBlockInfoLength(start_block, to_write_length);
      to_write_length = 0;
    }



    file_inode->SetSize(start_offset + write_length - to_write_length - 1);
    return Status::OK;
  }

  Status CreateFile(ServerContext* context, const CreateRequest* request,
                  CreateReply* reply) override{
    auto root_inode = inode_table_->GetInode(0);
    if (root_inode == nullptr) {
      std::cerr << __func__ << ": root dir's inode doesn't exist." << std::endl;
      return Status::CANCELLED;
    }
    auto filename = request->name();
    std::shared_ptr<Dentry> dentry;
    if ((dentry = root_inode->GetDentry(filename)) != nullptr) {
      std::cerr << __func__ << ": file " << filename << " already exist, inum" << std::endl;
      reply->set_inum(dentry->GetInodeNum());
      return Status::OK;
    }

    auto new_inum = inode_table_->AllocateFileInode();
    std::cout << __func__ << " alloc inode " << new_inum << " to file " << filename << std::endl;
    std::cout << __func__ << "inode table current size: " << inode_table_->GetCurSize() << std::endl;
    root_inode->AddDentry(filename, new_inum);
    reply->set_inum(new_inum);

    return Status::OK;
  }

  Status ListDirectory(ServerContext* context, const ListDirectoryRequest* request,
                  ListDirectoryReply* reply) override {
    /* The file system only has root directory now. */
    std::string path = request->path();
    auto root_inode = inode_table_->GetInode(0);
    if (root_inode == nullptr) {
      std::cerr << __func__ << ": root dir's inode doesn't exist." << std::endl;
      return Status::CANCELLED;
    }
    auto dentry_map = root_inode->GetDentryMap();
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


private:
  uint64_t CreateFile_(const std::string &name, bool is_dir) {
    auto root_inode = inode_table_->GetInode(0);
    if (root_inode == nullptr) {
      std::cerr << __func__ << ": root dir's inode doesn't exist." << std::endl;
      return inode_table_->GetTotalSize() + 1 ;
    }
    auto new_inum = inode_table_->AllocateFileInode();
    root_inode->AddDentry(name, new_inum);
    return new_inum;
  }

  std::shared_ptr<InodeTable> inode_table_;
  std::shared_ptr<SpaceManager> space_manager_;
};

void RunServer(uint16_t port) {
  std::string server_address = absl::StrFormat("0.0.0.0:%d", port);
  GreeterServiceImpl service;

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
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

ABSL_FLAG(uint16_t, port, 50051, "Server port for the service");
int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);
  RunServer(absl::GetFlag(FLAGS_port));
  return 0;
}