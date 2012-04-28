#include "HypothesisClient.h"
#include "HypothesisManager.h"
#include "../mynetwork.h"

std::map<int, dtCore::RefPtr<HypothesisClient> > HypothesisClient::mClients;
int HypothesisClient::mClientIndex = 100;

HypothesisClient::HypothesisClient(std::string name, HypothesisHost* host) :
	mName(name), mHost(host), mInit(false), mIdle(true)
{
	mId = mClientIndex++;
	mClients[mId] = this;
}

HypothesisClient::HypothesisClient(int id) : mHost(NULL), mInit(false), mId(id), mIdle(true)
{
	std::ostringstream oss;
	oss << "Instance " << id;
	mName = oss.str();
	//Note: if you specify one client id, you should probably specify all of them
	mClientIndex = id+1;
	mClients[id] = this;
}

HypothesisClient::~HypothesisClient()
{
	//close connection
	theApp->getNetwork()->DisconnectConnection(getAddress());
	//remove from mClients map
	LOG_ERROR("Hypothesis Client DIED!")
	mClients.erase(mId);
}

//STATIC member functions

void HypothesisClient::launchAllClients() {
	std::map<int, dtCore::RefPtr<HypothesisClient> >::iterator conns = mClients.begin();
	while(conns != mClients.end()) {
		conns->second->launch();
		//if (!theApp->getNetwork()->WaitForClient(conns->second.get())) {
		//	LOG_ERROR("Client could not be started");
			//TODO quit client process
		//}
		conns++;
	}
	LOG_INFO("All clients launched")
}

HypothesisClient *HypothesisClient::getClient(int id) {
	//return the client (returning mClients[id] directly seems to create a NULL entry)
	std::map<int, dtCore::RefPtr<HypothesisClient> >::iterator c = mClients.find(id);
	if (c != mClients.end()) {
		return c->second.get();
	} //else
	return NULL;
}

bool HypothesisClient::isClient(int id) {
	return (getClient(id)!= NULL);
}

void HypothesisClient::sendToAllClients(GNE::Packet& p) {
	std::map<int, dtCore::RefPtr<HypothesisClient> >::iterator it = mClients.begin();
	while(it != mClients.end()) {
		if (it->second->isInitialised())
			theApp->getNetwork()->SendPacket(it->second->getAddress(), p);
		++it;
	}
}

HypothesisClient* HypothesisClient::getIdleClient() {
	std::map<int, dtCore::RefPtr<HypothesisClient> >::iterator it;
	for (it = mClients.begin(); it != mClients.end(); it++) {
		if (it->second->isIdle()) {
			return it->second.get();
		}
	}
	return NULL;
}

/**
 * Returns a list of hypothesis client instances that are ready for new hypotheses
 * i.e both initialised and idle
 */
std::vector<HypothesisClient*> HypothesisClient::getIdleClients() {
	std::vector<HypothesisClient*> clients;
	std::map<int, dtCore::RefPtr<HypothesisClient> >::iterator it;
	for (it = mClients.begin(); it != mClients.end(); it++) {
		if (it->second->isIdle()) {
			clients.push_back(it->second.get());
		}
	}
	return clients;
}

//NON-STATIC member functions

bool HypothesisClient::launch() {
	if (!isInitialised() && mHost) {
		//launch process TODO better handling of paths
		std::ostringstream s;
		s << mHost->getExecString()+" --headless --id " << getId() << " --connect " << HypothesisManager::getHost() << " --port " << HypothesisManager::getPort() << " > clientLog" <<  getId() << ".txt 2>&1 &";
		//s << "cd ~/development/workspace/hammerQt/; ./hammerQt --id " << getId() << " --connect " << HypothesisManager::getHost() << " --port " << HypothesisManager::getPort() << " > clientLog" <<  getId() << ".txt 2>&1 &";

		std::string command;
		if (mHost->getHost() == "localhost" || mHost->getHost() == "127.0.0.1") {
			command = s.str();
		} else {
			command = "ssh "+mHost->getHost()+" \"" + s.str() + "\"";
		}
		int status = system(command.c_str());
		if (status == 0) {
			LOG_INFO("Launched: "+command);
			return true;
		}
	}
	return false;
}

bool HypothesisClient::isInitialised() {
	return mInit;
}

void HypothesisClient::setAddress(std::string address) {
	mAddress = address;
	mInit = true;
}

std::string HypothesisClient::getAddress() {
	return mAddress;
}

void HypothesisClient::run(std::vector<CommandPacket>& commandPackets, std::vector<dtCore::RefPtr<UnitSavePacket> >& unitPackets) {
	mIdle = false;

	//copy packets to heap and send in next MyNetwork::PreFrame because we sometimes (somehow) get a deadlock on the network mutex if sent directly from here
	for (std::vector<dtCore::RefPtr<UnitSavePacket> >::iterator it = unitPackets.begin(); it != unitPackets.end(); it++) {
		GNE::Packet* p = new UnitSavePacket(*it->get());
		theApp->getNetwork()->SendQueuedPacket(getAddress(), p);
	}

	for (std::vector<CommandPacket>::iterator it = commandPackets.begin(); it != commandPackets.end(); it++) {
		GNE::Packet* p = new CommandPacket(*it);
		theApp->getNetwork()->SendQueuedPacket(getAddress(), p);
	}

	LOG_DEBUG("Sent (queued) state and command packets")
}

void HypothesisClient::setIdle(bool idle) {
	mIdle = idle;
}

bool HypothesisClient::isIdle() {
	//return (isInitialised() && mCurrentHypothesis == NULL) || (isInitialised() && !mCurrentHypothesis->isAwaitingPredictions(mId));
	return (isInitialised() && mIdle);
}
