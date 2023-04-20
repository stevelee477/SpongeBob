#include "MetaClient.hpp"
#include "spongebob.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using spongebob::Greeter;
using spongebob::HelloReply;
using spongebob::HelloRequest;
using spongebob::ReadReply;
using spongebob::ReadRequest;
using spongebob::WriteReply;
using spongebob::WriteRequest;
using spongebob::ListDirectoryReply;
using spongebob::ListDirectoryRequest;
using spongebob::CreateRequest;
using spongebob::CreateReply;

using namespace spongebob;

GreeterClient::GreeterClient(std::shared_ptr<Channel> channel)
    : stub_(Greeter::NewStub(channel)) {}

// Assembles the client's payload, sends it and presents the response back
// from the server.
std::string GreeterClient::SayHello(const std::string &user) {
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

int GreeterClient::ReadFile(const std::string &filename, uint64_t offset,
                            uint64_t length) {
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
    return -1;
  }

  uint64_t size = read_reply.block_info_size();
  std::cout << __func__ << ": " << size << " file data blocks received." << std::endl;
  // auto data_list = read_reply.block_info();
  for (auto cur_block : read_reply.block_info()) {
    // std::cout << cur_block.get_serverid();
    std::cout << cur_block.serverid() << ", ";
    std::cout << cur_block.offset() << ", ";
    std::cout << cur_block.length() << std::endl;
  }
  std::cout << __func__ << ": " << "read file finished." << std::endl << std::endl;

  return 0;
}

int GreeterClient::WriteFile(const std::string &filename, uint64_t offset, uint64_t length) {
  WriteRequest write_request;
  write_request.set_name(filename);
  write_request.set_offset(offset);
  write_request.set_length(length);
  WriteReply write_reply;
  ClientContext context;
  Status status = stub_->WriteFile(&context, write_request, &write_reply);
  std::cout << __func__ << ": " << "write to file: "<< filename << ", offset: " << offset << ", length: " << length << std::endl;
  if (!status.ok()) {
    std::cout << status.error_code() << ": " << status.error_message()
              << std::endl;
    return -1;
  }

  uint64_t size = write_reply.block_info_size();
  std::cout << __func__ << ": " << size << " file data blocks info received." << std::endl;
  auto data_list = write_reply.block_info();

  for (auto cur_block : write_reply.block_info()) {
    // std::cout << cur_block.get_serverid();
    std::cout << cur_block.serverid() << ", ";
    std::cout << cur_block.offset() << ", ";
    std::cout << cur_block.length() << std::endl;
  }
  std::cout << __func__ << ": " << "write file finished." << std::endl << std::endl;
  return 0;
}

int GreeterClient::CreateFile(const std::string &filename) {
  CreateRequest create_request;
  create_request.set_name(filename);
  create_request.set_is_dir(false);
  CreateReply create_reply;
  ClientContext context;

  Status status = stub_->CreateFile(&context, create_request, &create_reply);

  return 0;
}

int GreeterClient::ListDirectory(const std::string &path) {
  ListDirectoryRequest list_dir_request;
  list_dir_request.set_path(path);
  ListDirectoryReply list_dir_reply;
  ClientContext context;
  Status status = stub_->ListDirectory(&context, list_dir_request, &list_dir_reply);

  if (!status.ok()) {
    std::cout << status.error_code() << ": " << status.error_message()
              << std::endl;
    return -1;
  }

  for (auto dentry_info: list_dir_reply.dentry_list()) {
    dentry_info.is_dir() ? std::cout << "directory, " : std::cout << "file: ";
    std::cout << dentry_info.name() << ", " << dentry_info.inum() << ", " << dentry_info.size() << std::endl;
  }

  return 0;
}

int GreeterClient::RegisterMemoryRegion(uint64_t nodeid, uint64_t addr, uint64_t length) {
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