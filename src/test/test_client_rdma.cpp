#include "RdmaSocket.hpp"
#include "MetaClient.hpp"
#include "Configuration.hpp"


void examineBuffer(char *buffer, size_t bufferSize) {
    for (size_t i = 0; i < bufferSize; ++i) {
        // Print each character in the buffer as a two-digit hexadecimal number
        std::cout << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(buffer[i])) << ' ';
    }
    std::cout << std::endl;
}

class TestRDMAClient {
public:
    TestRDMAClient();
private:
    std::unique_ptr<RdmaSocket> rdmaSocket;
};

TestRDMAClient::TestRDMAClient() {
    /*
    1. read config
    2. connect metaserver
    3, connect storageserver
    */
    Configuration *config = new Configuration();
    char *buffer = new char[100];
    rdmaSocket = std::make_unique<RdmaSocket>(2, reinterpret_cast<uint64_t>(buffer), 0, config, false, 0);
    rdmaSocket->RdmaConnect();
    // auto peer = rdmaSocket->getPeerInformation(1);

    // auto ret = rdmaSocket->RdmaWrite(1, peer->RegisteredMemory, reinterpret_cast<uint64_t>(buffer), 100, 0, 0);
    auto ret = rdmaSocket->RdmaRead(1, reinterpret_cast<uint64_t>(buffer), 0, 100, 0);
    if (ret) {
        std::cout << "write success" << std::endl;
    } else {
        std::cout << "write failed" << std::endl;
        exit(-1);
    }
    examineBuffer(buffer, 100);
    std::fill(buffer, buffer + 100, 1);
    examineBuffer(buffer, 100);
    ret = rdmaSocket->RdmaWrite(1, reinterpret_cast<uint64_t>(buffer), 0, 0x10000, 0, 0);
    ret = rdmaSocket->RdmaWrite(1, reinterpret_cast<uint64_t>(buffer), 0x1000, 0x3560, 0, 0);
    struct ibv_wc wc;
    rdmaSocket->PollCompletion(1, 2, &wc);
    ret = rdmaSocket->RdmaRead(1, reinterpret_cast<uint64_t>(buffer), 0, 100, 0);
    if (ret) {
        std::cout << "write success" << std::endl;
    } else {
        std::cout << "write failed" << std::endl;
        exit(-1);
    }
    examineBuffer(buffer, 100);
}


int main() {
    TestRDMAClient TestRDMAClient;
    return 0;
}