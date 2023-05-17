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

#include "RdmaSocket.hpp"
#include "Configuration.hpp"

ABSL_FLAG(std::string, target, "localhost:50051", "Server address");
ABSL_FLAG(std::string, user, "world", "The user's name");
using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using namespace spongebob;

struct BlockInfo {
  // BlockInfo() = default;
  // BlockInfo(uint64_t serverid, uint64_t offset, uint64_t length) : serverid(serverid), offset(offset), length(length) {}
  uint64_t serverid;
  uint64_t offset;
  uint64_t length;
};

class GreeterClient {
public:
  GreeterClient(std::shared_ptr<grpc::Channel> channel);
  std::string SayHello(const std::string &user);
  std::vector<BlockInfo> ReadFile(const std::string &filename, uint64_t offset, uint64_t length);
  std::vector<BlockInfo> WriteFile(const std::string &filename, uint64_t offset, uint64_t length);
  int CreateFile(const std::string &filename);
  int CreateDiretory(const std::string &path);
  int ListDirectory(const std::string &path);
  int RegisterMemoryRegion(uint64_t nodeid, uint64_t addr, uint64_t length);
private:
  std::unique_ptr<spongebob::Greeter::Stub> stub_;
  std::unique_ptr<RdmaSocket> rdmaSocket;
  std::unique_ptr<char[]> buffer;
};



GreeterClient::GreeterClient(std::shared_ptr<Channel> channel)
    : stub_(Greeter::NewStub(channel)) {}

// Assembles the client's payload, sends it and presents the response back
// from the server.
std::string GreeterClient::SayHello(const std::string &user) {
  HelloRequest request;
  request.set_name(user);
  HelloReply reply;
  ClientContext context;

  Status status = stub_->SayHello(&context, request, &reply);

  if (status.ok()) {
    return reply.message();
  } else {
    std::cout << status.error_code() << ": " << status.error_message()
              << std::endl;
    return "RPC failed";
  }
}

std::vector<BlockInfo> GreeterClient::ReadFile(const std::string &filename, uint64_t offset,
                            uint64_t length) {
  ReadRequest read_request;
  // int res = 0;
  read_request.set_name(filename);
  read_request.set_offset(offset);
  read_request.set_length(length);
  ReadReply read_reply;
  ClientContext context;

  Status status = stub_->ReadFile(&context, read_request, &read_reply);

  if (!status.ok()) {
    std::cout << status.error_code() << ": " << status.error_message()
              << std::endl;
    return std::vector<BlockInfo>();
  }

  uint64_t size = read_reply.block_info_size();
  std::cout << __func__ << ": " << size << " file data blocks received." << std::endl;

  size_t local_offset = 0;
  std::vector<BlockInfo> block_info_list;
  for (auto cur_block : read_reply.block_info()) {
    std::cout << cur_block.block_idx() << ", ";
    std::cout << cur_block.serverid() << ", ";
    std::cout << cur_block.mem_offset() << ", ";
    std::cout << cur_block.length() << ", ";
    std::cout << cur_block.buff_offset() << std::endl;
    // block_info_list.emplace_back(BlockInfo{cur_block.serverid(), cur_block.offset(), cur_block.length()});
  }
  std::cout << __func__ << ": " << "read file finished." << std::endl << std::endl;
  return block_info_list;
}

std::vector<BlockInfo> GreeterClient::WriteFile(const std::string &filename, uint64_t offset, uint64_t length) {
  WriteRequest write_request;
  write_request.set_name(filename);
  write_request.set_offset(offset);
  write_request.set_length(length);
  WriteReply write_reply;
  ClientContext context;
  Status status = stub_->WriteFile(&context, write_request, &write_reply);
  std::cout << __func__ << ": " << "write to file: "<< filename << ", offset: " << offset << ", length: " << length << std::endl;
  if (!status.ok()) {
    std::cout << status.error_code() << ": " << status.error_message()
              << std::endl;
    return std::vector<BlockInfo>();
  }

  uint64_t size = write_reply.block_info_size();
  std::cout << __func__ << ": " << size << " file data blocks info received." << std::endl;

  std::vector<BlockInfo> block_info_list;
  for (auto cur_block : write_reply.block_info()) {
    // std::cout << cur_block.get_serverid();
    std::cout << cur_block.block_idx() << ", ";
    std::cout << cur_block.serverid() << ", ";
    std::cout << cur_block.mem_offset() << ", ";
    std::cout << cur_block.length() << ", ";
    std::cout << cur_block.buff_offset() << std::endl;
    block_info_list.emplace_back(BlockInfo{cur_block.serverid(), cur_block.offset(), cur_block.length()});
    // auto ret = rdmaSocket->RdmaWrite(1, peer->RegisteredMemory, reinterpret_cast<uint64_t>(buffer),100, 0, 0);
  }
  std::cout << __func__ << ": " << "write file finished." << std::endl << std::endl;
  return block_info_list;
}

int GreeterClient::CreateFile(const std::string &filename) {
  CreateRequest create_request;
  create_request.set_name(filename);
  create_request.set_is_dir(false);
  CreateReply create_reply;
  ClientContext context;

  Status status = stub_->CreateFile(&context, create_request, &create_reply);

  return 0;
}

int GreeterClient::ListDirectory(const std::string &path) {
  ListDirectoryRequest list_dir_request;
  list_dir_request.set_path(path);
  ListDirectoryReply list_dir_reply;
  ClientContext context;
  Status status = stub_->ListDirectory(&context, list_dir_request, &list_dir_reply);

  if (!status.ok()) {
    std::cout << status.error_code() << ": " << status.error_message()
              << std::endl;
    return -1;
  }

  for (auto dentry_info: list_dir_reply.dentry_info()) {
    dentry_info.is_dir() ? std::cout << "directory, " : std::cout << "file: ";
    std::cout << dentry_info.name() << ", " << dentry_info.inum() << ", " << dentry_info.size() << std::endl;
  }

  return 0;
}

class Client {
public:
  Client();
  int read(std::string& filename, char* buffer, uint64_t offset, uint64_t length);
  int write(std::string& filename, char* buffer, uint64_t offset, uint64_t length);
  int create(std::string& filename);
  int list(std::string& path);
private:
  std::unique_ptr<GreeterClient> metaClient;
  std::unique_ptr<RdmaSocket> rdmaSocket;
  std::unique_ptr<char[]> buffer;
  int readTaskID = 0;
  int writeTaskID = 0;
};

Client::Client() {
    Configuration *config = new Configuration();
    buffer = std::make_unique<char[]>(1<<10);
    rdmaSocket = std::make_unique<RdmaSocket>(2, reinterpret_cast<uint64_t>(buffer.get()), 0, config, false, 0);
    rdmaSocket->RdmaConnect();
    metaClient = std::make_unique<GreeterClient>(grpc::CreateChannel(config->metaip, grpc::InsecureChannelCredentials()));
}

int Client::read(std::string& filename, char* user_buf, uint64_t offset, uint64_t length) {
  size_t local_offset = 0;

  auto block_info_list = metaClient->ReadFile(filename, offset, length);
  for (auto& block : block_info_list) {
    // auto ret = rdmaSocket->RdmaRead(block.serverid, reinterpret_cast<uint64_t>(buffer.get()) + local_offset, block.offset, block.length, readTaskID++);
    auto ret = rdmaSocket->RdmaRead(1, reinterpret_cast<uint64_t>(buffer.get()) + local_offset, block.offset, block.length, readTaskID++);
    if (!ret) {
      cerr << "read error" << endl;
      return -1;
    }
    local_offset += block.length;
    cout << __func__ << "\n";
  }
  assert(local_offset == length);
  memcpy(user_buf, buffer.get(), length);
  return 0;
}

int Client::write(std::string& filename, char* user_buf, uint64_t offset, uint64_t length) {
  size_t local_offset = 0;

  auto block_info_list = metaClient->WriteFile(filename, offset, length);
  for (auto& block : block_info_list) {
    // auto ret = rdmaSocket->RdmaWrite(block.serverid, reinterpret_cast<uint64_t>(user_buf) + local_offset, block.offset, block.length, 0, readTaskID++);
    auto ret = rdmaSocket->RdmaWrite(1, reinterpret_cast<uint64_t>(user_buf) + local_offset, block.offset, block.length, 0, readTaskID++);
    if (!ret) {
      cerr << "write error" << endl;
      return -1;
    }
    local_offset += block.length;
  }
  assert(local_offset == length);
  return 0;
}

int Client::create(std::string& filename) {
  return metaClient->CreateFile(filename);
}

int Client::list(std::string& path) {
  return metaClient->ListDirectory(path);
}

// int main(int argc, char** argv) {
//   absl::ParseCommandLine(argc, argv);
//   std::string target_str = absl::GetFlag(FLAGS_target);
//   std::string user = absl::GetFlag(FLAGS_user);
//   // We indicate that the channel isn't authenticated (use of
//   // InsecureChannelCredentials()).
//   GreeterClient greeter(
//       grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));
//   // std::string user("world");
//   // while (std::cin >> user) {
//   //   std::string reply = greeter.SayHello(user);
//   //   std::cout << "Greeter " << user << " received: " << reply << std::endl;
//   // }
//   // auto reply = greeter.Read();
//   // std::string reply = greeter.SayHello(user);
//   // std::cout << "Greeter " << user << " received: " << reply << std::endl;
//   std::srand(std::time(nullptr));

//   for (int i = 0; i < 3; ++i) {
//     std::string filename = "test" + std::to_string(i) + ".txt";
//     greeter.CreateFile(filename);
//   }
//   // auto l1 = 23368;
//   // auto offset = 22295;
//   // auto l2= 2222;
//   // greeter.WriteFile("test.txt", 0, l1);
//   // greeter.ReadFile("test.txt", 0, l1);
//   // greeter.WriteFile("test.txt", offset, l2);
//   // greeter.ReadFile("test.txt", offset, l2);
//   // greeter.ReadFile("test.txt", 0, offset + l2);

//   for (int i = 0; i < 3; ++i) {
//     std::string filename = "test" + std::to_string(i) + ".txt";
//     auto l1 = rand() % (1 << 15);
//     auto offset = rand() % l1;
//     auto l2= rand() % (1 << 14);
//     greeter.WriteFile(filename, 0, l1);
//     greeter.ReadFile(filename, 0, l1);
//     greeter.WriteFile(filename, offset, l2);
//     greeter.ReadFile(filename, offset, l2);
//     greeter.ReadFile(filename, 0, offset + l2);
//   }
//   greeter.ListDirectory("/");
//   // auto ret = greeter.ReadFile("test.txt", 0, 100);
//   return 0;
// }

int GreeterClient::RegisterMemoryRegion(uint64_t nodeid, uint64_t addr, uint64_t length) {
  RegisterMemoryRegionRequest register_memory_region_request;
  register_memory_region_request.set_nodeid(nodeid);
  register_memory_region_request.set_addr(addr);
  register_memory_region_request.set_length(length);
  RegisterMemoryRegionReply register_memory_region_reply;
  ClientContext context;
  Status status = stub_->RegisterMemoryRegion(&context, register_memory_region_request, &register_memory_region_reply);

  if (!status.ok()) {
    std::cout << status.error_code() << ": " << status.error_message()
              << std::endl;
    return -1;
  }
  return 0;
}

int main(int argc, char** argv) {
  absl::ParseCommandLine(argc, argv);
  std::string target_str = absl::GetFlag(FLAGS_target);
  std::string user = absl::GetFlag(FLAGS_user);
  Client client;
  // std::string user("world");
  // while (std::cin >> user) {
  //   std::string reply = greeter.SayHello(user);
  //   std::cout << "Greeter " << user << " received: " << reply << std::endl;
  // }
  // auto reply = greeter.Read();
  // std::string reply = greeter.SayHello(user);
  // std::cout << "Greeter " << user << " received: " << reply << std::endl;
  std::srand(std::time(nullptr));

  for (int i = 0; i < 3; ++i) {
    std::string filename = "test" + std::to_string(i) + ".txt";
    client.create(filename);
  }
  // auto l1 = 23368;
  // auto offset = 22295;
  // auto l2= 2222;
  // greeter.WriteFile("test.txt", 0, l1);
  // greeter.ReadFile("test.txt", 0, l1);
  // greeter.WriteFile("test.txt", offset, l2);
  // greeter.ReadFile("test.txt", offset, l2);
  // greeter.ReadFile("test.txt", 0, offset + l2);
  char* buf = new char[1 << 15];
  memset(buf, -1, 1 << 15);

  for (int i = 0; i < 3; ++i) {
    std::string filename = "test" + std::to_string(i) + ".txt";
    auto l1 = rand() % (1 << 15);
    auto offset = rand() % l1;
    auto l2= rand() % (1 << 14);
    client.write(filename, buf, 0, l1);
    client.read(filename, buf, 0, l1);
    cout << "read 1" << buf[0] << endl;
    client.write(filename, buf, offset, l2);
    client.read(filename, buf, offset, l2);
    client.read(filename, buf, 0, offset + l2);
    break;
  }
  // client.ListDirectory("/");
  // auto ret = greeter.ReadFile("test.txt", 0, 100);
  return 0;
}
