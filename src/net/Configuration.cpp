/***********************************************************************
* 
* 
* Tsinghua Univ, 2016
*
***********************************************************************/

#include "Configuration.hpp"
#include "debug.hpp"

using namespace std;

Configuration::Configuration() {
	ServerCount = 0;
	read_xml("../conf/conf.xml", pt);
	ptree child = pt.get_child("address.storage");
	for(BOOST_AUTO(pos,child.begin()); pos != child.end(); ++pos) 
    {  
        id2ip[(uint16_t)(pos->second.get<int>("id"))] = pos->second.get<string>("ip");
        ip2id[pos->second.get<string>("ip")] = pos->second.get<int>("id");
        ServerCount += 1;
    }
	metaip = pt.get<std::string>("address.meta");
	rdmaPort = pt.get<int>("address.rdma.port");
	rdmaTcpPort = pt.get<int>("address.rdma.tcp");
	std::cout << rdmaPort << " " << rdmaTcpPort << std::endl;
}

Configuration::~Configuration() {
	Debug::notifyInfo("Configuration is closed successfully.");
}

string Configuration::getIPbyID(uint16_t id) {
	return id2ip[id];
}

uint16_t Configuration::getIDbyIP(string ip) {
	return ip2id[ip];
}

unordered_map<uint16_t, string> Configuration::getInstance() {
	return id2ip;
}

int Configuration::getServerCount() {
	return ServerCount;
}
