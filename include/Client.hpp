#pragma once

#include "RdmaSocket.hpp"
#include "Configuration.hpp"
#include "MetaClient.hpp"

#include "spongebob.pb.h"

#include "spongebob.grpc.pb.h"

class Client {
public:
  Client();
  int read(const std::string &filename, char *buffer, uint64_t offset,
           uint64_t length);
  int write(const std::string &filename, const char *buffer, uint64_t offset,
            uint64_t length);
  int create(std::string &filename);
  // int list(std::string &path);
  std::shared_ptr<MetadataClient> getMetaClient() { return metaClient; }

private:
  std::shared_ptr<MetadataClient> metaClient;
  std::unique_ptr<RdmaSocket> rdmaSocket;
  std::unique_ptr<char[]> buffer;
};