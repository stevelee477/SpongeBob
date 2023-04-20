#pragma once

#include <string>
#include "RdmaSocket.hpp"
#include "pool.hpp"
#include "Configuration.hpp"
#include "MetaClient.hpp"

class StorageServer {
public:
    StorageServer(std::string device, size_t size);
private:
    std::unique_ptr<RdmaSocket> rdmaSocket;
    std::unique_ptr<Pool> pool;
    std::unique_ptr<GreeterClient> metaClient;
};