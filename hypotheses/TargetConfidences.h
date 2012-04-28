/*
 * GroupConfidences.h
 *
 *  Created on: 8 Jan 2010
 *      Author: sbutler
 */

#ifndef GROUPCONFIDENCES_H_
#define GROUPCONFIDENCES_H_

#include <osg/Referenced>
#include <dtCore/refptr.h>
#include <map>
#include <vector>

class UnitGroup;

class TargetConfidences : public osg::Referenced {
public:
	TargetConfidences(std::vector<dtCore::RefPtr<UnitGroup> > targetGroups);
	virtual ~TargetConfidences();

	void addUnitError(int unitID, float endTime, float error);
	void addGroupError(int groupID, float endTime, float error);

	float getAverageConfidence(int groupID, float windowSize = 20);
	float getLatestConfidence(int groupID);

	float getHighestConfidence();
	int getTargetGroup();
	std::vector<int> getTargetGroups();
	void getWinners(std::vector<int> &groups, float &maxConf);

private:
	std::vector<dtCore::RefPtr<UnitGroup> > mTargetGroups;
	//map unitIDs back to group indices
	std::map<int, int> mUnitsToGroup;
	//save errors
	std::map<int, std::map<float, float> > mGroupErrors;
};

#endif /* GROUPCONFIDENCES_H_ */
