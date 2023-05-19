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

  char *buf = new char[1 << 15];
  memset(buf, -1, 1 << 15);

  std::string filename = "test" + std::to_string(1) + ".txt";
  for (int i = 0; i < 10000; i++)
    buf[i] = 0xcc;
  client.write(filename, buf, 0, 10000);
  for (int i = 0; i < 1000; i++)
    buf[i] = 0x00;
  client.read(filename, buf, 0, 10000);
  cout << "read " << std::hex << static_cast<int>(buf[5000]) << endl;

  return 0;
}