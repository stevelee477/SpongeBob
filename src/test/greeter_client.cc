#include <cstdint>
#include <grpcpp/client_context.h>
#include <grpcpp/support/status.h>
#include <iostream>
#include <memory>
#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "spongebob.pb.h"

#include <grpcpp/grpcpp.h>
#include "spongebob.grpc.pb.h"

ABSL_FLAG(std::string, target, "localhost:50051", "Server address");
ABSL_FLAG(std::string, user, "world", "The user's name");
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using spongebob::Greeter;
using spongebob::HelloReply;
using spongebob::HelloRequest;
using spongebob::ReadRequest;
using spongebob::ReadReply;
using spongebob::WriteRequest;
using spongebob::WriteReply;

class GreeterClient {
 public:
  GreeterClient(std::shared_ptr<Channel> channel)
      : stub_(Greeter::NewStub(channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  std::string SayHello(const std::string& user) {
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

  int ReadFile(const std::string& filename, uint64_t offset, uint64_t length) {
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
      for (auto cur_block: read_reply.data_list()) {
        // std::cout << cur_block.get_serverid();
        std::cout << cur_block.serverid() << std::endl;
        std::cout << cur_block.offset() << std::endl;
        std::cout << cur_block.length() << std::endl << std::endl;
      }
      return 0;
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return -1;;
    }

    return res;
  }
 private:
  std::unique_ptr<Greeter::Stub> stub_;
};

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
  std::string reply = greeter.SayHello(user);
  std::cout << "Greeter " << user << " received: " << reply << std::endl;
  auto ret = greeter.ReadFile("test.txt", 0, 100);
  return 0;
}
