#include "Hypothesis.h"

#include "../Command.h"
#include "../packets.h"
#include "HypothesisManager.h"
#include "HypothesisClient.h"
#include "HypothesisResults.h"
#include "../eventlog.h"
#include "TargetConfidences.h"

#include <boost/lexical_cast.hpp>

int Hypothesis::mCount = 0;

/**
 * blueUnits are targets(groups)
 * redUnits are the (single) group of units being predicted
 */
Hypothesis::Hypothesis(std::vector<UnitGroup*>& blueUnits, UnitGroup* redUnits):
	mIdle(true),
	mCommandIndex(-1),
	mDuration(5),
	mStartTime(0),
	mRedUnits(redUnits),
	mFinishedTime(0)
{
	for (unsigned int i=0; i < blueUnits.size(); i++) {
		//wrap in a refptr
		mBlueUnits.push_back(blueUnits[i]);
	}
	mTargetConfidences = new TargetConfidences(mBlueUnits);
	mID = mCount++;
	mName = "H"+boost::lexical_cast<std::string>(mID);
}

Hypothesis::~Hypothesis()
{
}

dtCore::RefPtr<UnitGroup> Hypothesis::getObservationGroup() {
	return mRedUnits;
}

std::vector<dtCore::RefPtr<UnitGroup> > Hypothesis::getTargetGroups() {
	return mBlueUnits;
}

void Hypothesis::nextCommand() {
	if (allCommandsSent()) {
		update();
		mCommandIndex = getStartCommandIndex();
		//emit paramsUpdate(mParams);
	} else {
		mCommandIndex = getNextCommandIndex(mCommandIndex);
	}
}

std::vector<CommandPacket> Hypothesis::getCurrentCommand(HypothesisClient* client) {
	std::vector<CommandPacket> p;
	//increment command
	nextCommand();
	//save client / index pair
	mIndex[client] = mCommandIndex;
	//get params and groups TODO sort out groups/params
	//for (unsigned int i=0; i < mGroups.size(); i++) {
		p.push_back(CommandPacket(mCommands.at(mCommandIndex), mParams.at(mCommandIndex), mGroups.at(mCommandIndex), mDuration, mTimeScale));
	//}
	//TODO get blue units commands from sim
	std::vector<int> ids;
	for (unsigned int i=0; i < mBlueUnits.size(); i++) {
		std::vector<int> groupIds = mBlueUnits[i]->getUnitIDs();
		std::copy(groupIds.begin(), groupIds.end(), std::back_inserter(ids));
	}
	p.push_back(CommandPacket(Command::NONE, ids, mDuration, mTimeScale));
	//TODO also send the red units that we're not actually using in this hypothesis
	//because it will affect the BLUE units targeting
	return p;
}

bool Hypothesis::allCommandsSent() {
	return mCommandIndex >= getEndCommandIndex();
}

int Hypothesis::getCommandIndex(int index) {
	//find end
	while (index < (signed)mCommands.size() && mCommandsEnabled[index] == false) {
		index++;
	}
	return index;
}

int Hypothesis::getStartCommandIndex() {
	return getCommandIndex(0);
}

int Hypothesis::getNextCommandIndex(int index) {
	return getCommandIndex(index + 1);
}

int Hypothesis::getEndCommandIndex() {
	//find end
	int end = mCommands.size()-1;
	while (end >= 0 && mCommandsEnabled[end] == false) {
		end--;
	}
	return end;
}

void Hypothesis::addCommand(int command, std::vector<int>& group, osg::Vec3 params, std::string paramDescription, std::vector<int> targetUnits) {
	int id = mCommands.size();
	mCommands.push_back((Command::CommandType)command);
	mCommandsEnabled.push_back(true);
	mGroups.push_back(group);
	mParams.push_back(params);
	mParamChanges[id][HypothesisManager::getManager()->getTimeStep()] = paramDescription;
	mTargetUnits.push_back(targetUnits);
	mResults.push_back(new HypothesisResults());
}

/**
 * Initialises n commands with the supplied command and group.
 * updateParam must be called to fill in the parameters before the command can be executed
 */
void Hypothesis::initCommands(int n, int command, std::vector<int>& group) {
	for (int i=0; i < n; i++) {
		mCommands.push_back((Command::CommandType)command);
		mCommandsEnabled.push_back(true);
		mGroups.push_back(group);
		mResults.push_back(new HypothesisResults());
	}
	mParams.resize(n);
	mTargetUnits.resize(n);
}

void Hypothesis::clearCommands() {
	mCommands.clear();
	mCommandsEnabled.clear();
	mGroups.clear();
	mParams.clear();
	mTargetUnits.clear();
	mCommandsEnabled.clear();
}

void Hypothesis::updateCommand(int id, int command, std::vector<int>& group, osg::Vec3 params, std::string paramDescription, std::vector<int> targetUnits) {
	mCommands[id] = (Command::CommandType)command;
	mGroups[id] = group;
	mParams[id] = params;
	mParamChanges[id][HypothesisManager::getManager()->getTimeStep()] = paramDescription;
	mTargetUnits[id] = targetUnits;
	mCommandsEnabled[id] = true;
}

void Hypothesis::updateParam(int id, osg::Vec3 params, std::string paramDescription, std::vector<int> targetUnits) {
	mParams[id] = params;
	mParamChanges[id][HypothesisManager::getManager()->getTimeStep()] = paramDescription;
	mTargetUnits[id] = targetUnits;
	mCommandsEnabled[id] = true;
}

void Hypothesis::disableCommand(int id) {
	mCommandsEnabled[id] = false;
}

std::vector<Command::CommandType> Hypothesis::getAllCommands() {
	return mCommands;
}

std::vector<std::vector<int> > Hypothesis::getAllGroups() {
	return mGroups;
}

std::vector<osg::Vec3> Hypothesis::getAllParams() {
	return mParams;
}

/**
 * Saves a snapshot of the start state and the starting time
 * Then triggers the sending of the state and commands to the client to start the simulation
 */
void Hypothesis::start(HypothesisClient* c, std::vector<Unit*>& units) {
	//get command packets
	std::vector<CommandPacket> commandPackets = getCurrentCommand(c);

	if (mCommandIndex == getStartCommandIndex()) {
		//serialise units
		mStartUnitPackets.clear();
		mStartUnits.clear();
		for (std::vector<Unit*>::iterator it = units.begin(); it != units.end(); it++) {
			//make a copy of the unit (only if it is a local unit, otherwise it is already a snapshot)
			if (theApp->isLocalInstance((*it)->getClientID())) {
				mStartUnits.push_back((*it)->clone());
			} else {
				mStartUnits.push_back(*it);
			}
			dtCore::RefPtr<UnitSavePacket> p = new UnitSavePacket(*it, units.size());
			mStartUnitPackets.push_back(p);
		}

		mStartTime = HypothesisManager::getManager()->getTimeStep();

		mGotActual = false;
		mGotPredictions.clear();
		mActualUnits.clear();
		mUnitPredictions.clear();
	}

	startCommand(c, commandPackets);
}

/**
 * Call this to start the next command with the state saved from the call to 'start'
 */
void Hypothesis::startNextCommand(HypothesisClient* c) {
	//get command packets
	std::vector<CommandPacket> commandPackets = getCurrentCommand(c);
	startCommand(c, commandPackets);
}

/**
 * send the commands to the client
 */
void Hypothesis::startCommand(HypothesisClient* c, std::vector<CommandPacket>& commandPackets) {
	LOG_INFO("Starting hypothesis " + getName() + ", command " + boost::lexical_cast<std::string>(mCommandIndex))

	//send packets
	c->run(commandPackets, mStartUnitPackets);

	mGotPredictions[c->getId()] = false;

	if (allCommandsSent()) {
		//stop the execution of a new command sequence until results are in
		mIdle = false;
	} else {
		mIdle = true;
	}
}

bool Hypothesis::isValid() {
	if (mRedUnits->isValidGroup()) {
		//also make sure at least one of the blueunit groups is valid
		for (unsigned int i=0; i < mBlueUnits.size(); i++) {
			if (mBlueUnits[i]->isValidGroup()) {
				return true;
			}
		}
	}
	return false;
}

bool Hypothesis::isAwaitingActual() {
	return !mGotActual;
}

bool Hypothesis::isAwaitingPredictions(int client) {
	std::map<int,int>::iterator it = mGotPredictions.find(client);
	if (it != mGotPredictions.end()) {
		return !(it->second);
	}
	return false;
}

int Hypothesis::getFinishingTime() {
	return mStartTime + mDuration;
}

void Hypothesis::reset() {
	mIdle = true;
	mGotActual = false;
	mGotPredictions.clear();
}

void Hypothesis::savePredictions(HypothesisClient* client, std::vector<Unit*>& units) {
	//get index from client id
	int id = mIndex[client];
	for (std::vector<Unit*>::iterator it = units.begin(); it != units.end(); it++) {
		mUnitPredictions[id][(*it)->getUnitID()] = *it;
	}
	mGotPredictions[client->getId()] = true;
	setIdle();
}

void Hypothesis::saveActual(std::vector<Unit*>& units) {
	mGotActual = true;
	for (std::vector<Unit*>::iterator it = units.begin(); it != units.end(); it++) {
		mActualUnits[(*it)->getUnitID()] = (*it)->clone();
	}
	setIdle();
}

void Hypothesis::setIdle() {
	if (allCommandsSent() && mGotActual) {
		for (std::map<int,int>::iterator it = mGotPredictions.begin(); it != mGotPredictions.end(); it++) {
			if (it->second == false) {
				return;
			}
		}
		calcConfidence();
		saveResults();
		//get new winning target groups after we've calculated the new confidences
		std::vector<int> newTargetGroups = mTargetConfidences->getTargetGroups();
		//if new groups are not in old groups then this is a new target
		if (!mTargetGroups.empty() && !newTargetGroups.empty()) {
			bool changed = true;
			for (unsigned int i=0; i < newTargetGroups.size(); i++) {
				for (unsigned int j=0; j < mTargetGroups.size(); j++) {
					if (newTargetGroups[i] == mTargetGroups[j]) {
						//there is the same group in the new target groups and the old target groups
						changed = false;
						break;
					}
				}
				if (!changed) break;
			}
			if (changed) {
				//log change in target
				float deviation = getDeviation(getObservationGroup()->getCentreCurrent());
				std::ostringstream oss;
				oss << "TARG " << getID() << " " << newTargetGroups[0] << " " << ((deviation < 0) ? NAN : deviation);
				EventLog::GetInstance().logEvent(oss.str());
			}
		}
		mTargetGroups = newTargetGroups;
		//tell the hypothesis manager we've finished and calculated all our confidences
		HypothesisManager::get()->hypothesisFinished(this);
		mFinishedTime = theApp->getSimTime();
		mIdle = true;
	}
}

void Hypothesis::calcConfidence() {
	//loop through all predictions to get individual unit confidences
	for (std::map<int,std::map<int,dtCore::RefPtr<Unit> > >::iterator pit = mUnitPredictions.begin(); pit != mUnitPredictions.end(); pit++) {
		int id = pit->first;
		std::map<int,dtCore::RefPtr<Unit> >& predictedUnits = pit->second;
		//calc for each individual unit
		float error = 0;
		float errorCount = 0;
		for (std::vector<dtCore::RefPtr<Unit> >::iterator it = mStartUnits.begin(); it != mStartUnits.end(); it++) {
			int unit = (*it)->getUnitID();

			std::map<int,dtCore::RefPtr<Unit> >::iterator actualUnit = mActualUnits.find(unit);
			std::map<int,dtCore::RefPtr<Unit> >::iterator predictedUnit = predictedUnits.find(unit);

			//make sure we have data for this unit (if not then its probably dead, so skip)
			if ((actualUnit == mActualUnits.end()) || (predictedUnit == predictedUnits.end())) {
				//mResults.at(id)->addResult(unit, 0, startTime, duration);
				continue;
			}

			//get positions
			osg::Vec3 start = (*it)->getPosition();
			osg::Vec3 actual = actualUnit->second->getPosition() - start;
			osg::Vec3 predicted = predictedUnit->second->getPosition() - start;

			std::cout << "s: " << start << " a: " << actualUnit->second->getPosition() << " p: " << predicted << std::endl;

			float actlen = actual.length();
			float predlen = predicted.length();
			float mindist = 1.0;
			float distconf = (actlen > predlen) ? (predlen / actlen) : (actlen / predlen);

			float confidence = 0;

			if (actlen < mindist && predlen < mindist) {
				//if no movement then confidence is high
				confidence = 1;
			} else {
				//normalise
				actual.normalize();
				predicted.normalize();

				//dot product (angle)
				confidence = actual * predicted;
				//scale by normalised difference in distance
				confidence *= distconf;
			}

			//std::cout << unit << "> s: " << start << " a: " << actualUnit->second->m_Position << " p: " << predictedUnit->second->m_Position << " c: " << confidence << std::endl;
			dtUtil::Log::GetInstance().LogMessage(dtUtil::Log::LOG_INFO,__FUNCTION__, __LINE__,"Confidence for %s is %f",(*it)->GetName().c_str(),confidence);

			//save
			mResults.at(id)->addResult(unit, confidence, mStartTime, mDuration);

			error += confidence;
			errorCount++;
		}

		//Save the average error going to the targets
		float avError = error / errorCount;
		float endTime = mStartTime + mDuration;

		for (unsigned int i=0; i < mTargetUnits[id].size(); i++) {
			mTargetConfidences->addUnitError(mTargetUnits[id][i], endTime, avError);
		}
	}
	//TODO calculate specific confidences for each command,
	//e.g. 	formation - average (except leader)
	//		manouvre - if in formation: leader, else average
}

HypothesisResults *Hypothesis::getResultsModel(int index) {
	return mResults.at(index).get();
}

/*int Hypothesis::getWinningCommand() {
	if (mResults.empty()) return -1;
	//get max confidence of all the commands
	float maxconf = std::numeric_limits<float>::denorm_min();
	int maxI = -1;
	for (unsigned int i=0; i < mCommands.size(); i++) {
		float conf = mResults.at(i)->getConfidence();
		if (conf > maxconf) {
			maxconf = conf;
			maxI = i;
		}
	}
	return maxI;
}*/

/**
 * Gets the results of the command with the highest confidence
 *
HypothesisResults* Hypothesis::getWinningCommandResults() {
	int command = getWinningCommand();
	if (command >= 0)
		return mResults.at(command).get();
	return NULL;
}*/

UnitGroup* Hypothesis::getTargetGroup() {
	int groupIndex = mTargetConfidences->getTargetGroup();

	if (groupIndex >= 0) {
		return mBlueUnits[groupIndex].get();
	} else {
		EventLog::GetInstance().logEvent("UNIX no winning target group");
	}
	return NULL;
}

/**
 * Returns the IDs of the units who are the target of the latest winning command
 */
std::vector<int> Hypothesis::getTargetUnits() {
	std::vector<int> g = mTargetConfidences->getTargetGroups();

	std::vector<int> u;
	for (unsigned int i=0; i < g.size(); i++) {
		if (g[i] >= 0) {
			std::vector<int> unew = mBlueUnits[g[i]].get()->getUnitIDs();
			u.insert(u.end(), unew.begin(), unew.end());
		} else {
			EventLog::GetInstance().logEvent("UNIX no units in group");
		}
	}
	return u;

	/*if (command) {
		HypothesisResult* result = command->getResult();
		if (result) {
			std::vector<int> units = result->getTargetUnits()->getUnitIDs();
			if (units.size() > 0) {
				return units[0];
			} else {
				std::ostringstream oss;
				oss << "UNIT " << getID() << " " << target << " " << theApp->getUnitById(target)->getName();
				EventLog::GetInstance().logEvent(oss.str());

				EventLog::GetInstance().logEvent("UNIX no units");
			}
		} else {
			EventLog::GetInstance().logEvent("UNIX no result");
		}
	} else {
		EventLog::GetInstance().logEvent("UNIX no winning target");
	}*/
	return std::vector<int>();
}

/**
 * Gets the latest highest confidence of this hypothesis (the max confidence of all the commands)
 */
float Hypothesis::getConfidence() {
	/*if (!mResults.empty()) {
		HypothesisResults *results = getWinningCommandResults();
		if (results)
			return results->getConfidence();
	}*/
	//find max confidence to each target group
	return mTargetConfidences->getHighestConfidence();
}

/**
 * Gets the latest confidence for all of the commands in this hypothesis
 *
std::vector<float> Hypothesis::getConfidences() {
	std::vector<float> confidences;
	for (unsigned int i=0; i < mCommands.size(); i++) {
		float conf = mResults.at(i)->getConfidence();
		confidences.push_back(conf);
	}
	return confidences;
}*/

/**
 * Saves relevant info for presentation in the GUI.
 * Override this to show a path.
 */
void Hypothesis::saveResults() {
	//save the overall result for each command
	for (unsigned int i=0; i < mCommands.size(); i++) {
		if (mCommandsEnabled[i]) {
			float conf = mResults.at(i)->getGroupError();
			//add a new result object for this command
			mResults.at(i)->addResult(new HypothesisResult(getName(),mRedUnits,conf,mStartTime,mTargetUnits[i]));
		}
	}
}

std::vector<HypothesisResult*> Hypothesis::getLatestResults() {
	std::vector<HypothesisResult*> r;
	for (unsigned int i=0; i < mCommands.size(); i++) {
		if (mCommandsEnabled[i]) {
			HypothesisResult* res = mResults.at(i)->getResult();
			if (res != NULL) {
				r.push_back(res);
			}
		}
	}
	return r;
}

/**
 * For logging purposes, get the confidences for all units
 */
std::string Hypothesis::getResultsString() {
	std::ostringstream f;
	for (unsigned int i=0; i < mResults.size(); i++) {
		//header
		f << "Hypothesis\t" << getID() << "\tCommand\t" << i << std::endl;
		if (mParamChanges.size() > 0) {
			f << "Parameters\t";
			std::map<int,std::string> params = mParamChanges[i];
			for(std::map<int,std::string>::iterator it = params.begin(); it != params.end(); it++) {
				//list of (time index):(identifier);...
				f << it->first << ":" << it->second << ";";
			}
			f << std::endl;
		}
		//results
		f << mResults.at(i)->getResults();
	}
	return f.str();
}

/**
 * Gets the priority for this hypothesis needing another run.
 * Lower numbers have higher priority.
 */
double Hypothesis::getPriority() {
	double tMax = 320; //hypotheses should be run at least every 320 seconds
	double tDiff = theApp->getSimTime() - mFinishedTime;
	//max confidence of 1, with a linear negative slope, crossing the x-axis at tMax
	double tPriority = (1 - tDiff/tMax);
	//tConf = (tConf < 0) ? 0 : tConf; //go below the x-axis?

	double conf = getConfidence();
	//give a low priority if the confidence is either very high or very low
	//linear. '\/'
	double confPriority = fabs(conf);

	return tPriority + confPriority;
	//return tConf * (getConfidence() + 1);
}

/**
 * Gets the distance from pos to the destination
 */
float Hypothesis::getDeviation(osg::Vec3 pos) {
	UnitGroup* g = getTargetGroup();
	if (g) {
		osg::Vec3 dest = g->getCentreCurrent();

		return (dest - pos).length();
	}
	return 0;
}

/**
 * Returns the time difference since the last time this hypothesis has finished executing
 */
double Hypothesis::getTimeDiff() {
	return theApp->getSimTime() - mFinishedTime;
}

/**
 * Returns the maximum threat level of all of the commands
 * 1 = Threatening
 * 0 = Non-threatening
 *
float Hypothesis::getThreat() {

}*/

/**
 * The (approx) amount of time it takes to simulate this hypothesis
 */
float Hypothesis::getSimTime() {
	return (mDuration / mTimeScale) * mCommands.size();
}
