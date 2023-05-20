#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <grpcpp/client_context.h>
#include <grpcpp/support/status.h>
#include <iostream>
#include <memory>
#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "spongebob.pb.h"

#include "spongebob.grpc.pb.h"
#include <grpcpp/grpcpp.h>

#include "Client.hpp"

ABSL_FLAG(std::string, target, "localhost:50051", "Server address");
ABSL_FLAG(std::string, user, "world", "The user's name");

int main(int argc, char **argv) {
  absl::ParseCommandLine(argc, argv);
  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created. This channel models a connection to an endpoint specified by
  // the argument "--target=" which is the only expected argument.
  std::string target_str = absl::GetFlag(FLAGS_target);
  std::string user = absl::GetFlag(FLAGS_user);
  Client client;

  std::srand(std::time(nullptr));

  for (int i = 0; i < 3; ++i) {
    std::string filename = "test" + std::to_string(i) + ".txt";
    client.create(filename);
  }

  char *buf = new char[1 * 1024 * 1024 * 1024];
  // memset(buf, -1, 1 << 20);

  std::string filename = "test" + std::to_string(1) + ".txt";
  for (int i = 0; i < 100000; i++)
    buf[i] = 0xcc;
  client.write(filename, buf, 0, 20000);
  for (int i = 0; i < 10000; i++)
    buf[i] = 0x00;
  client.read(filename, buf, 0, 20000);
  cout << "read " << std::hex << static_cast<int>(buf[5000]) << endl;

  std::srand(std::time(nullptr));


  // // auto l1 = 23368;
  // // auto offset = 22295;
  // // auto l2= 2222;
  // // greeter.WriteFile("test.txt", 0, l1);
  // // greeter.ReadFile("test.txt", 0, l1);
  // // greeter.WriteFile("test.txt", offset, l2);
  // // greeter.ReadFile("test.txt", offset, l2);
  // // greeter.ReadFile("test.txt", 0, offset + l2);

  // for (int i = 0; i < 3; ++i) {
  //   std::string filename = "test" + std::to_string(i) + ".txt";
  //   greeter.CreateFile(filename);
  // }
  // std::cout << std::endl;
  // for (int i = 0; i < 3; ++i) {
  //   std::string filename = "test" + std::to_string(i) + ".txt";
  //   greeter.RemoveFile(filename);
  // }
  // std::cout << std::endl;

  // char buffer[1024];
  // for (int i = 0; i < 3; ++i) {
  //   std::string filename = "test" + std::to_string(i) + ".txt";
  //   greeter.OpenFile(filename);
  // }
  // std::cout << std::endl;
  // for (int i = 0; i < 3; ++i) {
  //   std::string filename = "test" + std::to_string(i) + ".txt";
  //   greeter.CreateFile(filename);
  // }
  // std::cout << std::endl;
  // for (int i = 0; i < 3; ++i) {
  //   std::string filename = "test" + std::to_string(i) + ".txt";
  //   greeter.RemoveFile(filename);
  // }

  // std::cout << std::endl;
  // for (int i = 0; i < 3; ++i) {
  //   std::string filename = "test" + std::to_string(i) + ".txt";
  //   auto l1 = rand() % (1 << 15);
  //   auto offset = rand() % l1;
  //   auto l2= rand() % (1 << 14);
  //   greeter.WriteFile(filename, 0, l1, buffer);
  //   greeter.ReadFile(filename, 0, l1, buffer);
  //   greeter.WriteFile(filename, offset, l2, buffer);
  //   greeter.ReadFile(filename, offset, l2, buffer);
  //   greeter.ReadFile(filename, 0, offset + l2, buffer);
  // }

  // std::vector<struct dentry_info> dentry_list;
  // greeter.ReadDirectory("/", dentry_list);
  // greeter.RemoveFile("test1.txt");
  // std::cout << std::endl;

  // greeter.ReadDirectory("/", dentry_list);
  // greeter.RemoveFile("test2.txt");

  // std::cout << std::endl;
  // greeter.ReadDirectory("/", dentry_list);
  // // auto ret = greeter.ReadFile("test.txt", 0, 100);


  return 0;
}