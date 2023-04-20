#include <cstdint>
#include <grpcpp/client_context.h>
#include <grpcpp/support/status.h>
#include <iostream>
#include <memory>
#include <string>
#include <cstdlib>
#include <iostream>
#include <ctime>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "spongebob.pb.h"

#include "spongebob.grpc.pb.h"
#include <grpcpp/grpcpp.h>

ABSL_FLAG(std::string, target, "localhost:50051", "Server address");
ABSL_FLAG(std::string, user, "world", "The user's name");
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using namespace spongebob;
// using spongebob::Greeter;
// using spongebob::HelloReply;
// using spongebob::HelloRequest;
// using spongebob::ReadReply;
// using spongebob::ReadRequest;
// using spongebob::WriteReply;
// using spongebob::WriteRequest;
// using spongebob::ListDirectoryReply;
// using spongebob::ListDirectoryRequest;
// using spongebob::CreateRequest;
// using spongebob::CreateReply;

class GreeterClient {
public:
  GreeterClient(std::shared_ptr<grpc::Channel> channel);
  std::string SayHello(const std::string &user);
  int ReadFile(const std::string &filename, uint64_t offset, uint64_t length);
  int WriteFile(const std::string &filename, uint64_t offset, uint64_t length);
  int CreateFile(const std::string &filename);
  int CreateDiretory(const std::string &path);
  int ListDirectory(const std::string &path);
private:
  std::unique_ptr<spongebob::Greeter::Stub> stub_;
};



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
  std::cout << __func__ << ": " << filename << " created, inum: " << create_reply.inum() << std::endl;
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

  for (auto dentry_info: list_dir_reply.dentry_info()) {
    dentry_info.is_dir() ? std::cout << "directory, " : std::cout << "file: ";
    std::cout << dentry_info.name() << ", inode num: " << dentry_info.inum() << ", size: " << dentry_info.size() << std::endl;
  }

  return 0;
}

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);
  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created. This channel models a connection to an endpoint specified by
  // the argument "--target=" which is the only expected argument.
  std::string target_str = absl::GetFlag(FLAGS_target);
  std::string user = absl::GetFlag(FLAGS_user);
  // We indicate that the channel isn't authenticated (use of
  // InsecureChannelCredentials()).
  GreeterClient greeter(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
  // std::string user("world");
  // while (std::cin >> user) {
  //   std::string reply = greeter.SayHello(user);
  //   std::cout << "Greeter " << user << " received: " << reply << std::endl;
  // }
  // auto reply = greeter.Read();
  // std::string reply = greeter.SayHello(user);
  // std::cout << "Greeter " << user << " received: " << reply << std::endl;
  std::srand(std::time(nullptr));

  for (int i = 0; i < 3; ++i) {
    std::string filename = "test" + std::to_string(i) + ".txt";
    greeter.CreateFile(filename);
  }
  // auto l1 = 23368;
  // auto offset = 22295;
  // auto l2= 2222;
  // greeter.WriteFile("test.txt", 0, l1);
  // greeter.ReadFile("test.txt", 0, l1);
  // greeter.WriteFile("test.txt", offset, l2);
  // greeter.ReadFile("test.txt", offset, l2);
  // greeter.ReadFile("test.txt", 0, offset + l2);

  for (int i = 0; i < 3; ++i) {
    std::string filename = "test" + std::to_string(i) + ".txt";
    auto l1 = rand() % (1 << 15);
    auto offset = rand() % l1;
    auto l2= rand() % (1 << 14);
    greeter.WriteFile(filename, 0, l1);
    greeter.ReadFile(filename, 0, l1);
    greeter.WriteFile(filename, offset, l2);
    greeter.ReadFile(filename, offset, l2);
    greeter.ReadFile(filename, 0, offset + l2);
  }
  greeter.ListDirectory("/");
  // auto ret = greeter.ReadFile("test.txt", 0, 100);
  return 0;
}
