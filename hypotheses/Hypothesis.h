#ifndef HYPOTHESIS_H_
#define HYPOTHESIS_H_

#include <osg/Vec3>
#include <osg/Referenced>
#include <vector>
#include <map>
#include <dtCore/refptr.h>
#include <QObject>

#include "../Command.h"
#include "../units/Unit.h"
#include "../units/UnitGroup.h"

class CommandPacket;
class HypothesisClient;
class HypothesisResults;
class HypothesisResult;
class TargetConfidences;

class Hypothesis : public QObject, public osg::Referenced
{
	Q_OBJECT
public:
	Hypothesis(std::vector<UnitGroup*>& blueUnits, UnitGroup* redUnits);
	virtual ~Hypothesis();

	void setDuration(int d) { mDuration = d; }
	void setTimeScale(double s) { mTimeScale = s; }

	dtCore::RefPtr<UnitGroup> getObservationGroup();
	std::vector<dtCore::RefPtr<UnitGroup> > getTargetGroups();

	void nextCommand();
	//creates the command packet for each group
	std::vector<CommandPacket> getCurrentCommand(HypothesisClient* client);
	bool allCommandsSent();
	//initialises the vectors that store the command parameters to size n
	void initCommands(int n, int command, std::vector<int>& group);
	//adds a new commmand to the set, called by init
	void addCommand(int command, std::vector<int>& group, osg::Vec3 params = osg::Vec3(NAN,NAN,NAN), std::string paramDescription = "", std::vector<int> targetUnits = std::vector<int>());
	//(unused)
	void clearCommands();
	//update the command (unused)
	void updateCommand(int id, int command, std::vector<int>& group, osg::Vec3 params, std::string paramDescription, std::vector<int> targetUnits);
	//update the parameters, called by update
	void updateParam(int id, osg::Vec3 params, std::string paramDescription, std::vector<int> targetUnits);
	//if command invalid, then disable
	void disableCommand(int id);

	//get the data for all commands in the command set
	std::vector<Command::CommandType> getAllCommands();
	std::vector<std::vector<int> > getAllGroups();
	std::vector<osg::Vec3> getAllParams();

	//set the commands
	virtual void init() {};
	//
	virtual void start(HypothesisClient* c, std::vector<Unit*>& units);
	void startNextCommand(HypothesisClient* c);
	//update the parameters and the groups
	virtual void update() {};
	//generate a confidence for each group
	virtual void calcConfidence();
	//calculates a priority from the confidence and last run time
	virtual double getPriority();

	//check that the units in this hypothesis are still here
	bool isValid();

	bool isAwaitingActual();
	bool isAwaitingPredictions(int client);
	int getFinishingTime();
	void reset();

	void savePredictions(HypothesisClient* client, std::vector<Unit*>& units);
	void saveActual(std::vector<Unit*>& units);

	void setIdle();
	bool isIdle() { return mIdle; }

	//HypothesisResults* getWinningCommandResults();
	UnitGroup* getTargetGroup();
	std::vector<int> getTargetUnits();
	//gets the max average confidence of all units in each command
	float getConfidence();
	//std::vector<float> getConfidences();

	std::string getResultsString();
	HypothesisResults* getResultsModel(int index);

	std::string getName() { return mName; }
	int getID() { return mID; }

	virtual void saveResults();
	std::vector<HypothesisResult*> getLatestResults();

	virtual float getDeviation(osg::Vec3 pos);

	double getTimeDiff();

	float getSimTime();

/*signals:
	void paramsUpdate(std::vector<osg::Vec3> params);*/

protected:
	void startCommand(HypothesisClient* c, std::vector<CommandPacket>& commandPackets);
	int getWinningCommand();

	int getCommandIndex(int index);
	int getStartCommandIndex();
	int getNextCommandIndex(int index);
	int getEndCommandIndex();

	static int mCount;

	bool mIdle;
	int mCommandIndex;
	int mDuration;
	double mTimeScale;
	double mStartTime;
	bool mGotActual;
	std::map<int, int> mGotPredictions;

	std::vector< Command::CommandType > mCommands;	//TODO something with partitions and groups...
	std::vector<int> mCommandsEnabled;
	std::vector<dtCore::RefPtr<UnitGroup> > mBlueUnits;
	dtCore::RefPtr<UnitGroup> mRedUnits;

	//keep track of which commands get assigned to which clients
	std::map<HypothesisClient*, int> mIndex;

	std::vector< std::vector<int> > mGroups;
	std::vector<osg::Vec3> mParams;

	//mUnitPredictions[commandIndex][unitId] = Unit*
	std::map<int,std::map<int,dtCore::RefPtr<Unit> > > mUnitPredictions;

	std::vector<dtCore::RefPtr<Unit> > mStartUnits;
	std::vector<dtCore::RefPtr<UnitSavePacket> > mStartUnitPackets;
	std::map<int,dtCore::RefPtr<Unit> > mActualUnits;

	//one ClientResults per command (e.g. per formation)
	std::vector<dtCore::RefPtr<HypothesisResults> > mResults;

	//mParamChanges[commandIndex][time index] = "parameter description string" (written out in log file)
	std::map<int,std::map<int,std::string> > mParamChanges;

	//target units for each command
	std::vector<std::vector<int> > mTargetUnits;
	//the target groups with the highest confidence
	std::vector<int> mTargetGroups;

	//keep track of group-level confidences
	dtCore::RefPtr<TargetConfidences> mTargetConfidences;

	double mFinishedTime;

	std::string mName;
	int mID;
};

#endif /*HYPOTHESIS_H_*/
