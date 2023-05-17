#include "StorageServer.hpp"

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