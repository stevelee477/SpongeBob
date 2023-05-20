#pragma once
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

using FileBlockInfo = spongebob::FileBlockInfo;

struct dentry_info {
  std::string name;
  bool is_dir;
  uint64_t inum;
  uint64_t size;
};


class GreeterClient {
public:
  GreeterClient(std::shared_ptr<grpc::Channel> channel);
  std::string SayHello(const std::string &user);
  std::vector<FileBlockInfo> ReadFile(const std::string &filename, uint64_t offset, uint64_t length, uint64_t &bytes_read, char* buffer=nullptr);
  std::vector<FileBlockInfo> WriteFile(const std::string &filename, uint64_t offset, uint64_t length, uint64_t &bytes_write, const char* buffer=nullptr);
  int CreateFile(const std::string &filename);
  int CreateDiretory(const std::string &path);
  int OpenFile(const std::string &filename);
  bool CloseFile(int64_t fd);
  bool RemoveFile(const std::string &filename);
  int ReadDirectory(const std::string &path, std::vector<struct dentry_info>& dentry_list);
  int RegisterMemoryRegion(const uint64_t nodeid, const uint64_t addr, const uint64_t length);
private:
  std::unique_ptr<spongebob::Greeter::Stub> stub_;
};