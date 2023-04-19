#include <grpcpp/server_context.h>
#include <iostream>
#include <memory>
#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
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

using namespace spongebob;


// Logic and data behind the server's behavior.
class GreeterServiceImpl final : public Greeter::Service {
public:
  Status SayHello(ServerContext* context, const HelloRequest* request,
                  HelloReply* reply) override {
    std::string prefix("Hello ");
    reply->set_message(prefix + request->name());
    return Status::OK;
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

    for (int i = 0; i < 3; ++i) {
      auto cur_block = reply->add_data_list();
      cur_block->set_serverid(i);
      cur_block->set_offset(i * 2);
      cur_block->set_length(i * 3);
    }
    return Status::OK;
  }

  Status WriteFile(ServerContext* context, const WriteRequest* request,
                   WriteReply* reply) override {

    return Status::OK;
  }

  Status ListDirectory(ServerContext* context, const ListDirectoryRequest* request,
                  ListDirectoryReply* reply) {


  }

  Status RegisterMemoryRegion(ServerContext* context, const RegisterMemoryRegionRequest* request,
                  RegisterMemoryRegionReply* reply) {
    std::cout << "RegisterMemoryRegion: " << request->nodeid() << " 0x" << std::hex << request->addr() << std::dec << " Size: " << request->length() << std::endl;
    return Status::OK;
  }


private:
  InodeTable* inode_table_ = nullptr;
  SpaceManager* space_manager_ = nullptr;
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
