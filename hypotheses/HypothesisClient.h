#ifndef CLIENT_H_
#define CLIENT_H_

#include <algorithm>
#include <string>
#include <osg/Referenced>
#include <QMetaType>

#include "../common.h"
#include "../packets.h"
#include "Hypothesis.h"

class HypothesisManager;

class HypothesisHost {
public:
	HypothesisHost(std::string host="localhost", int cores=1, std::string path="~/development/workspace/hammerQt", std::string exec="hammerQt"):
		mHost(host), mCores(cores) {
		mExecString = "cd "+path+"; ./"+exec;
	};
	std::string getHost() { return mHost; };
	int getNumCores() { return mCores; };
	std::string getExecString() { return mExecString; };
private:
	std::string mHost;
	int mCores;
	std::string mExecString;
};

class HypothesisClient : public osg::Referenced
{
public:
	HypothesisClient() {};
	HypothesisClient(std::string name, HypothesisHost* host);
	HypothesisClient(int);
	virtual ~HypothesisClient();

	static void launchAllClients();
	static HypothesisClient* getClient(int);
	static bool isClient(int);
	static void sendToAllClients(GNE::Packet& p);
	static HypothesisClient* getIdleClient();
	static std::vector<HypothesisClient*> getIdleClients();

	bool launch();
	bool isInitialised();
	void setAddress(std::string address);
	std::string getName() { return mName; }
	int getId() { return mId; }
	std::string getAddress();

	void run(std::vector<CommandPacket>& commandPackets, std::vector<dtCore::RefPtr<UnitSavePacket> >& unitPackets);

	Hypothesis* getCurrentHypothesis() { return mCurrentHypothesis.get(); }
	void setIdle(bool idle);
	bool isIdle();

	static std::map<int,dtCore::RefPtr<HypothesisClient> > mClients;

private:

	static int mClientIndex;
	std::string mName;
	HypothesisHost* mHost;
	bool mInit;
	int mId;
	std::string mAddress;
	bool mIdle;

	dtCore::RefPtr<Hypothesis> mCurrentHypothesis;
};

Q_DECLARE_METATYPE(HypothesisClient*)

#endif /*CLIENT_H_*/
