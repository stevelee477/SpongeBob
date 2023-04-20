#include <cstdint>
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

class GreeterClient {
public:
  GreeterClient(std::shared_ptr<grpc::Channel> channel);
  std::string SayHello(const std::string &user);
  int ReadFile(const std::string &filename, uint64_t offset, uint64_t length);
  int WriteFile(const std::string &filename, uint64_t offset, uint64_t length);
  int CreateFile(const std::string &filename);
  int CreateDiretory(const std::string &path);
  int ListDirectory(const std::string &path);
  int RegisterMemoryRegion(const uint64_t nodeid, const uint64_t addr, const uint64_t length);
private:
  std::unique_ptr<spongebob::Greeter::Stub> stub_;
};