#include "StorageServer.hpp"

Pool::Pool(std::string dev, size_t _size) : size(_size) {
    dev_fd = open(dev.c_str(), O_RDWR);
    if (dev_fd < 0) {
        std::perror("open");
    }
    void *mmptr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, 0);
    if (mmptr == MAP_FAILED) {
        std::perror("mmap");
    }
    MemoryBaseAddress = reinterpret_cast<uint64_t>(mmptr);
}

Pool::~Pool() {
    munmap(reinterpret_cast<void *>(MemoryBaseAddress), size);
    close(dev_fd);
}

StorageServer::StorageServer(std::string device, size_t size) {
    /*
        1. read config
        2. create pool
        3. register RDMA region(maybe another class)
        4. notify metaserver the address
    */
    Configuration *config = new Configuration();
    pool = std::make_unique<Pool>(device, size);
    rdmaSocket = std::make_unique<RdmaSocket>(2, pool->getMemoryBaseAddress(), pool->getSize(), config, true, 0);
    rdmaSocket->RdmaListen();
    metaClient = std::make_unique<GreeterClient>(grpc::CreateChannel(config->metaip, grpc::InsecureChannelCredentials()));
    metaClient->RegisterMemoryRegion(rdmaSocket->getNodeID(), pool->getMemoryBaseAddress(), pool->getSize());
}