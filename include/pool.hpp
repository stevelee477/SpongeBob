#pragma once

#include "RdmaSocket.hpp"
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

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