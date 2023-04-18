#ifndef RPCCLINET_HREADER
#define RPCCLINET_HREADER
#include <thread>
#include "RdmaSocket.hpp"
#include "Configuration.hpp"
#include "mempool.hpp"
#include "global.h"
#include <cstdint>
#include <grpcpp/client_context.h>
#include <grpcpp/support/status.h>
#include <iostream>
#include <memory>
#include <string>

#include "MetaClient.hpp"

using namespace std;
class RPCClient {
private:
	Configuration *conf;
	RdmaSocket *socket;
	MemoryManager *mem;
	bool isServer;
	uint32_t taskID;
	GreeterClient *metaclient;
public:
	uint64_t mm;
	RPCClient(Configuration *conf, RdmaSocket *socket, MemoryManager *mem, uint64_t mm);
	RPCClient();
	~RPCClient();
	RdmaSocket* getRdmaSocketInstance();
	Configuration* getConfInstance();
	bool RdmaCall(uint16_t DesNodeID, char *bufferSend, uint64_t lengthSend, char *bufferReceive, uint64_t lengthReceive);
	uint64_t ContractSendBuffer(GeneralSendBuffer *send);
	string metaHello();
};

#endif