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
#include "MetaClient.hpp"

ABSL_FLAG(std::string, target, "localhost:50055", "Server address");
ABSL_FLAG(std::string, user, "world", "The user's name");
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using namespace spongebob;


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
    greeter.CreateFile(filename);
  }
  std::cout << std::endl;
  for (int i = 0; i < 3; ++i) {
    std::string filename = "test" + std::to_string(i) + ".txt";
    greeter.RemoveFile(filename);
  }
  std::cout << std::endl;

  char buffer[1024];
  for (int i = 0; i < 3; ++i) {
    std::string filename = "test" + std::to_string(i) + ".txt";
    greeter.OpenFile(filename);
  }
  std::cout << std::endl;
  for (int i = 0; i < 3; ++i) {
    std::string filename = "test" + std::to_string(i) + ".txt";
    greeter.CreateFile(filename);
  }
  std::cout << std::endl;
  for (int i = 0; i < 3; ++i) {
    std::string filename = "test" + std::to_string(i) + ".txt";
    greeter.RemoveFile(filename);
  }

  std::cout << std::endl;
  for (int i = 0; i < 3; ++i) {
    std::string filename = "test" + std::to_string(i) + ".txt";
    auto l1 = rand() % (1 << 15);
    auto offset = rand() % l1;
    auto l2= rand() % (1 << 14);
    greeter.WriteFile(filename, 0, l1, buffer);
    greeter.ReadFile(filename, 0, l1, buffer);
    greeter.WriteFile(filename, offset, l2, buffer);
    greeter.ReadFile(filename, offset, l2, buffer);
    greeter.ReadFile(filename, 0, offset + l2, buffer);
  }

  std::vector<struct dentry_info> dentry_list;
  greeter.ReadDirectory("/", dentry_list);
  greeter.RemoveFile("test1.txt");
  std::cout << std::endl;

  greeter.ReadDirectory("/", dentry_list);
  greeter.RemoveFile("test2.txt");

  std::cout << std::endl;
  greeter.ReadDirectory("/", dentry_list);
  // auto ret = greeter.ReadFile("test.txt", 0, 100);
  return 0;
}