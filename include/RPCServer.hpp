#ifndef RPCSERVER_HREADER
#define RPCSERVER_HREADER
#include <thread>
#include <unordered_map>
#include <vector>
#include "RdmaSocket.hpp"
#include "Configuration.hpp"
#include "RPCClient.hpp"
#include "mempool.hpp"
#include "global.h"
#include "filesystem.hpp"
#include "TxManager.hpp"

#include <grpcpp/server_context.h>
#include <iostream>
#include <memory>
#include <string>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include "spongebob.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;
using spongebob::Greeter;
using spongebob::HelloReply;
using spongebob::HelloRequest;
using spongebob::ReadRequest;
using spongebob::ReadReply;

using namespace std;

typedef unordered_map<uint32_t, int> Thread2ID;

typedef struct {
	uint64_t send;
	uint16_t NodeID;
	uint16_t offset;
} RPCTask;

class RPCServer {
private:
	thread *wk;
	Configuration *conf;
	RdmaSocket *socket;
	MemoryManager *mem;
	uint64_t mm;
	TxManager *tx;
	RPCClient *client;
	int ServerCount;
	FileSystem *fs;
	int cqSize;
	Thread2ID th2id;
	vector<RPCTask*> tasks;
	bool UnlockWait;
	void Worker(int id);
	void ProcessRequest(GeneralSendBuffer *send, uint16_t NodeID, uint16_t offset);
	void ProcessQueueRequest();
public:
	RPCServer(int cqSize);
	RdmaSocket* getRdmaSocketInstance();
	MemoryManager* getMemoryManagerInstance();
	RPCClient* getRPCClientInstance();
	TxManager* getTxManagerInstance();
	uint64_t ContractReceiveBuffer(GeneralSendBuffer *send, GeneralReceiveBuffer *recv);
	void RequestPoller(int id);
	int getIDbyTID();
	~RPCServer();
};

#endif