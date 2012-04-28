/*
 * GroupConfidences.cpp
 *
 *  Created on: 8 Jan 2010
 *      Author: sbutler
 */

#include "TargetConfidences.h"

#include "../units/UnitGroup.h"
#include "../common.h"
#include "../HTAppBase.h"

TargetConfidences::TargetConfidences(std::vector<dtCore::RefPtr<UnitGroup> > targetGroups) :
	mTargetGroups(targetGroups)
{
	for (unsigned int i=0; i < targetGroups.size(); i++) {
		std::vector<int> unitIDs = targetGroups[i]->getUnitIDs();
		for (unsigned int j=0; j < unitIDs.size(); j++) {
			mUnitsToGroup[unitIDs[j]] = i;
		}
	}
}

TargetConfidences::~TargetConfidences() {
}

/**
 * Save the error of going to this unit
 * The error is the predictive group average
 * (its many to one mapping from unitID to group, but the error should be the same for all units in a group)
 */
void TargetConfidences::addUnitError(int unitID, float endTime, float error) {
	mGroupErrors[mUnitsToGroup[unitID]][endTime] = error;
}

/**
 * Save group error
 */
void TargetConfidences::addGroupError(int groupID, float endTime, float error) {
	mGroupErrors[groupID][endTime] = error;
}

/**
 * Gets the latest confidence of the results
 * Averaged over a time window
 * //Will decay confidence if called at a time without a result
 */
float TargetConfidences::getAverageConfidence(int groupID, float windowSize) {
	//get average error over window size for enemy units
	//std::map<int, std::vector<std::pair<float,float> > >::iterator it;
	std::map<float, float>::reverse_iterator it;
	float acc = 0;
	//double currTime = theApp->getSimTime();

	std::map<int, std::map<float, float> >::iterator groupErrors = mGroupErrors.find(groupID);
	if (groupErrors != mGroupErrors.end() && !groupErrors->second.empty()) {
		double currTime = theApp->getSimTime();
		float winStartTime = currTime - windowSize;
		int samples = 0;
		//loop backwards through time index
		for (it = groupErrors->second.rbegin(); it != groupErrors->second.rend(); it++) {
			//get windowed error
			if (it->first > winStartTime) {
				acc += it->second;
				samples++;
			} else {
				break;
			}
		}
		if (samples > 0) {
			acc = acc / samples;
		}
		//decay with time
		/*double timeDiff = currTime - lastSampleTime;
		double decayTime = windowSize - timeDiff;
		if (decayTime < 0) decayTime = 0;
		float decayRatio = decayTime / windowSize;
		return average * decayRatio;*/
	}
	return acc;
}

float TargetConfidences::getLatestConfidence(int groupID) {
	std::map<int, std::map<float, float> >::iterator groupErrors = mGroupErrors.find(groupID);
	if (groupErrors != mGroupErrors.end() && !groupErrors->second.empty()) {
		std::map<float, float>::reverse_iterator it = groupErrors->second.rbegin();
		return it->second;
	}
	return 0;
}

/**
 * Get the highest confidence
 */
float TargetConfidences::getHighestConfidence() {
	std::vector<int> group;
	float conf;
	getWinners(group, conf);
	return conf;
}

/**
 * Get the winning group index
 */
int TargetConfidences::getTargetGroup() {
	std::vector<int> group;
	float conf;
	getWinners(group, conf);
	if (group.size() > 0) {
		return group[0];
	}
	return -1;
}

std::vector<int> TargetConfidences::getTargetGroups() {
	std::vector<int> group;
	float conf;
	getWinners(group, conf);
	return group;
}

/**
 * Get the winning group and confidence
 */
void TargetConfidences::getWinners(std::vector<int> &groups, float &maxConf) {
	maxConf = -std::numeric_limits<float>::max();
	for (unsigned int i=0; i < mTargetGroups.size(); i++) {
		if (!mTargetGroups[i]->isValidGroup()) continue;
		float conf = getLatestConfidence(i);
		if (conf == maxConf) {
			groups.push_back(i);
		} else if (conf > maxConf) {
			groups.clear();
			groups.push_back(i);
			maxConf = conf;
		}
	}
	if (groups.size() <= 0) {
		std::cout << "> Groups vector is inexplicably empty " << mTargetGroups.size() << std::endl;
	}
}
