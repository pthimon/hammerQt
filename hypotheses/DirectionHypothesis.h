#ifndef DIRECTIONHYPOTHESIS_H_
#define DIRECTIONHYPOTHESIS_H_

#include "Hypothesis.h"

class DirectionHypothesis : public Hypothesis
{
public:
	DirectionHypothesis(std::vector<UnitGroup*>& targetUnits, UnitGroup* myUnits, int numLevels, int numSegments);
	virtual ~DirectionHypothesis();

	virtual void init();
	virtual void update();

protected:
	float getAngleFromDepth(int depth);
	void allocateCommands();

	void calcUnitsAngles(osg::Vec3 heading);
	std::vector<int> getUnitsInSegment(float rotation, float interval);

	int mNumSegments;
	float mThreshold;
	int mMainSegment;
	int mLastIndex;
	int mLastIndexCount;
	int mDepth;
	int mLevels;

	std::map<int,float> mAccumulatedConfidences;
	std::vector<float> mAnglesToHeading;
};

#endif /*DIRECTIONHYPOTHESIS_H_*/
