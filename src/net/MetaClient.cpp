#include "MetaClient.hpp"

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
  int res = 0;
  read_request.set_name(filename);
  read_request.set_offset(offset);
  read_request.set_length(length);
  ReadReply read_reply;
  ClientContext context;
  Status status = stub_->ReadFile(&context, read_request, &read_reply);
  if (status.ok()) {
    uint64_t size = read_reply.data_list_size();
    std::cout << size << " file data blocks received." << std::endl;
    auto data_list = read_reply.data_list();
    for (auto cur_block : read_reply.data_list()) {
      // std::cout << cur_block.get_serverid();
      std::cout << cur_block.serverid() << std::endl;
      std::cout << cur_block.offset() << std::endl;
      std::cout << cur_block.length() << std::endl << std::endl;
    }
    return 0;
  } else {
    std::cout << status.error_code() << ": " << status.error_message()
              << std::endl;
    return -1;
    ;
  }

  return res;
}