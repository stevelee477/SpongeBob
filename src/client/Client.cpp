#include "Client.hpp"
#include <cstdint>

Client::Client() {
  Configuration *config = new Configuration();
  const int bufferSize = 1 * 1024 * 1024 * 1024;
  buffer = std::make_unique<char[]>(bufferSize);
  Debug::debugItem("Buffer start %lx size", buffer.get(), bufferSize);
  rdmaSocket =
      std::make_unique<RdmaSocket>(1, reinterpret_cast<uint64_t>(buffer.get()),
                                   bufferSize, config, false, 0);
  rdmaSocket->RdmaConnect();
  metaClient = std::make_unique<GreeterClient>(
      grpc::CreateChannel(config->metaip, grpc::InsecureChannelCredentials()));
}

int Client::read(std::string &filename, char *user_buf, uint64_t offset,
                 uint64_t length) {
  size_t local_offset = 0;
  uint64_t bytes_read = 0;
  auto block_info_list = metaClient->ReadFile(filename, offset, length, bytes_read);
  for (auto &block : block_info_list) {
    auto ret = rdmaSocket->RemoteRead(reinterpret_cast<uint64_t>(user_buf),
                                      block.serverid(), block.mem_offset(),
                                      block.length());
    if (!ret) {
      Debug::notifyError("Read error");
      return -1;
    }
    local_offset += block.length();
  }
  assert(local_offset == length);

  return 0;
}

int Client::write(std::string &filename, char *user_buf, uint64_t offset,
                  uint64_t length) {
  size_t local_offset = 0;
  uint64_t bytes_write = 0;
  auto block_info_list = metaClient->WriteFile(filename, offset, length, bytes_write);
  for (auto &block : block_info_list) {
    Debug::debugItem("Client Write Local buffer 0x%lx, serverid %d, "
                     "mem_offset %d, length %d, "
                     "buff_offset %d",
                     reinterpret_cast<uint64_t>(buffer.get()) +
                         block.buff_offset(),
                     block.serverid(), block.mem_offset(), block.length(),
                     block.buff_offset());
    auto ret = rdmaSocket->RemoteWrite(reinterpret_cast<uint64_t>(user_buf),
                                       block.serverid(), block.mem_offset(),
                                       block.length());
    if (!ret) {
      Debug::notifyError("Write error");
      return -1;
    }
    local_offset += block.length();
  }
  assert(local_offset == length);

  return 0;
}

int Client::create(std::string &filename) {
  return metaClient->CreateFile(filename);
}