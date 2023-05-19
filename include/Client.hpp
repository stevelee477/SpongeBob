#pragma once

#include "RdmaSocket.hpp"
#include "Configuration.hpp"
#include "MetaClient.hpp"

#include "spongebob.pb.h"

#include "spongebob.grpc.pb.h"

class Client {
public:
  Client();
  int read(std::string &filename, char *buffer, uint64_t offset,
           uint64_t length);
  int write(std::string &filename, char *buffer, uint64_t offset,
            uint64_t length);
  int create(std::string &filename);
  int list(std::string &path);

private:
  std::unique_ptr<GreeterClient> metaClient;
  std::unique_ptr<RdmaSocket> rdmaSocket;
  std::unique_ptr<char[]> buffer;
};