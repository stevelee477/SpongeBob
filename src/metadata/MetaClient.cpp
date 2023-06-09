#include "MetaClient.hpp"
#include "spongebob.pb.h"
#include "debug.hpp"
#include <cstdint>
#include <string>

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using namespace::spongebob;

MetadataClient::MetadataClient(std::shared_ptr<Channel> channel)
    : stub_(Metadata::NewStub(channel)) {}

// Assembles the client's payload, sends it and presents the response back
// from the server.
std::string MetadataClient::SayHello(const std::string &user) {
  // Data we are sending to the server.
  HelloRequest request;
  request.set_name(user);

  // Container for the data we expect from the server.
  HelloReply reply;

  // Context for the client. It could be used to convey extra information to
  // the server and/or tweak certain RPC behaviors.
  ClientContext context;

  // The actual RPC.
  Status status = stub_->SayHello(&context, request, &reply);

  // Act upon its status.
  if (status.ok()) {
    return reply.message();
  } else {
    std::cout << status.error_code() << ": " << status.error_message()
              << std::endl;
    return "RPC failed";
  }
}

std::vector<FileBlockInfo> MetadataClient::ReadFile(const std::string &filename, uint64_t offset,
                            uint64_t length, uint64_t &byted_read, char* buffer) {
  ReadRequest read_request;
  // int res = 0;
  read_request.set_name(filename);
  read_request.set_offset(offset);
  read_request.set_length(length);
  ReadReply read_reply;
  ClientContext context;
  Status status = stub_->ReadFile(&context, read_request, &read_reply);

  if (!status.ok()) {
    std::cout << status.error_code() << ": " << status.error_message()
              << std::endl;
    return std::vector<FileBlockInfo>();
  }
  byted_read = read_reply.bytes_read();
  Debug::debugItem("%s: Read %lu bytes from file %s.", __func__, byted_read, filename.c_str());
  std::vector<FileBlockInfo> block_info(read_reply.block_info().begin(), read_reply.block_info().end());
  return block_info;
  // uint64_t size = read_reply.block_info_size();
  // std::cout << __func__ << ": " << size << " file data blocks received." << std::endl;
  // // auto data_list = read_reply.block_info();
  // for (auto cur_block : read_reply.block_info()) {
  //   // std::cout << cur_block.get_serverid();
  //   std::cout << cur_block.block_idx() << ", ";
  //   std::cout << cur_block.serverid() << ", ";
  //   std::cout << cur_block.mem_offset() << ", ";
  //   std::cout << cur_block.length() << ", ";
  //   std::cout << cur_block.buff_offset() << std::endl;
  // }
  // std::cout << __func__ << ": " << "read file finished." << std::endl << std::endl;
}

std::vector<FileBlockInfo> MetadataClient::WriteFile(const std::string &filename, uint64_t offset, uint64_t length, uint64_t &bytes_write, const char* buffer) {
  WriteRequest write_request;
  write_request.set_name(filename);
  write_request.set_offset(offset);
  write_request.set_length(length);
  WriteReply write_reply;
  ClientContext context;

  Status status = stub_->WriteFile(&context, write_request, &write_reply);
  // std::cout << __func__ << ": " << "write to file: "<< filename << ", offset: " << offset << ", length: " << length << std::endl;
  // Debug::
  if (!status.ok()) {
    std::cout << status.error_code() << ": " << status.error_message()
              << std::endl;
    return std::vector<FileBlockInfo>();
  }
  std::vector<FileBlockInfo> block_info(write_reply.block_info().begin(), write_reply.block_info().end());
  bytes_write = write_reply.bytes_write();
  return block_info;
  // uint64_t size = write_reply.block_info_size();
  // std::cout << __func__ << ": " << size << " file data blocks info received." << std::endl;
  // auto data_list = write_reply.block_info();

  // for (auto cur_block : write_reply.block_info()) {
  //   // std::cout << cur_block.get_serverid();
  //   std::cout << cur_block.block_idx() << ", ";
  //   std::cout << cur_block.serverid() << ", ";
  //   std::cout << cur_block.mem_offset() << ", ";
  //   std::cout << cur_block.length() << ", ";
  //   std::cout << cur_block.buff_offset() << std::endl;
  // }
  // std::cout << __func__ << ": " << "write file finished." << std::endl << std::endl;
  // return 0;
}

int MetadataClient::CreateFile(const std::string &filename) {
  CreateRequest create_request;
  create_request.set_name(filename);
  create_request.set_is_dir(false);
  CreateReply create_reply;
  ClientContext context;

  Status status = stub_->CreateFile(&context, create_request, &create_reply);
  // std::cout << __func__ << ": create file " << filename << ", inum " << create_reply.inum() << std::endl;
  return 0;
}

int MetadataClient::CreateDiretory(const std::string &path) {
  CreateRequest create_request;
  create_request.set_name(path);
  create_request.set_is_dir(true);
  CreateReply create_reply;
  ClientContext context;

  Status status = stub_->CreateFile(&context, create_request, &create_reply);

  return 0;
}

int MetadataClient::OpenFile(const std::string &filename) {
  OpenRequest open_request;
  open_request.set_name(filename);
  open_request.set_is_dir(false);
  OpenReply open_reply;
  ClientContext context;

  Status status = stub_->OpenFile(&context, open_request, &open_reply);
  int fd = open_reply.fd();
  // std::cout << __func__ << ": open file " << filename << ", fd " << fd << std::endl;
  return fd;
}

bool MetadataClient::CloseFile(int64_t fd) {
  CloseRequest close_request;
  close_request.set_fd(fd);
  close_request.set_is_dir(false);
  CloseReply close_reply;
  ClientContext context;

  Status status = stub_->CloseFile(&context, close_request, &close_reply);
  bool success  = close_reply.success();
  // std::cout << __func__ << ": close file fd " << fd << ", status " << success << std::endl;
  return success;
}

bool MetadataClient::RemoveFile(const std::string &filename) {
  RemoveRequest remove_request;
  remove_request.set_name(filename);
  remove_request.set_is_dir(false);
  RemoveReply remove_reply;
  ClientContext context;

  Status status = stub_->RemoveFile(&context, remove_request, &remove_reply);
  bool success  = remove_reply.success();
  // std::cout << __func__ << ": remove file " << filename << ", status " << success << std::endl;
  return success;
}

int MetadataClient::ReadDirectory(const std::string &path, std::vector<struct dentry_info> &dir_list) {
  ReadDirectoryRequest list_dir_request;
  list_dir_request.set_path(path);
  ReadDirectoryReply list_dir_reply;
  ClientContext context;
  Status status = stub_->ReadDirectory(&context, list_dir_request, &list_dir_reply);

  if (!status.ok()) {
    std::cout << status.error_code() << ": " << status.error_message()
              << std::endl;
    return -1;
  }

  for (auto d_info: list_dir_reply.dentry_info()) {
    std::string name = d_info.name();
    dir_list.push_back({name, d_info.is_dir(), d_info.inum(), d_info.size()});
    // d_info.is_dir() ? std::cout << "directory, " : std::cout << "file: ";
    // std::cout << d_info.name() << ", " << d_info.inum() << ", " << d_info.size() << std::endl;
  }
  return 0;
}

int MetadataClient::RegisterMemoryRegion(uint64_t nodeid, uint64_t addr, uint64_t length) {
  RegisterMemoryRegionRequest register_memory_region_request;
  register_memory_region_request.set_nodeid(nodeid);
  register_memory_region_request.set_addr(addr);
  register_memory_region_request.set_length(length);
  RegisterMemoryRegionReply register_memory_region_reply;
  ClientContext context;
  Status status = stub_->RegisterMemoryRegion(&context, register_memory_region_request, &register_memory_region_reply);

  if (!status.ok()) {
    std::cout << status.error_code() << ": " << status.error_message()
              << std::endl;
    return -1;
  }
  return 0;
}