#pragma once

#include <string>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

#include "RdmaSocket.hpp"
#include "Configuration.hpp"
#include "MetaClient.hpp"

class Pool {
public:
  Pool(std::string dev, size_t _size);
  ~Pool();
  const uint64_t getMemoryBaseAddress() const { return MemoryBaseAddress; }
  const size_t getSize() const { return size; }
private:
  int dev_fd;
  size_t size;
  uint64_t MemoryBaseAddress;
};

class StorageServer {
public:
    StorageServer(std::string device, size_t size);
private:
    std::unique_ptr<RdmaSocket> rdmaSocket;
    std::unique_ptr<Pool> pool;
    std::unique_ptr<MetadataClient> metaClient;
};