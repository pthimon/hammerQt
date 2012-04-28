/*
 * TargetDirectionHypothesis.h
 *
 *  Created on: 18 Jan 2010
 *      Author: sbutler
 */

#ifndef TARGETDIRECTIONHYPOTHESIS_H_
#define TARGETDIRECTIONHYPOTHESIS_H_

#include "Hypothesis.h"

class TargetDirectionHypothesis: public Hypothesis {
public:
	TargetDirectionHypothesis(std::vector<UnitGroup*>& targetUnits, UnitGroup* myUnits);
	virtual ~TargetDirectionHypothesis();

	void update();
	void saveResults();

private:
	bool mInit;
	osg::Vec3 mStartPos;
	std::map<int, osg::Vec3> mEndPos;
};

#endif /* TARGETDIRECTIONHYPOTHESIS_H_ */
