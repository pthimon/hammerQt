/*
 * TargetDirectionHypothesis.cpp
 *
 *  Created on: 18 Jan 2010
 *      Author: sbutler
 */

#include "TargetDirectionHypothesis.h"

#include "HypothesisResults.h"
#include <boost/lexical_cast.hpp>

TargetDirectionHypothesis::TargetDirectionHypothesis(std::vector<UnitGroup*>& targetUnits, UnitGroup* myUnits):
	Hypothesis(targetUnits, myUnits),
	mInit(false)
{
}

TargetDirectionHypothesis::~TargetDirectionHypothesis() {
}

void TargetDirectionHypothesis::update() {
	//called when we have confidences for each separate command
	//update the parameters and the groups

	if (!mInit) {
		std::vector<int> ids = mRedUnits->getUnitIDs();
		initCommands(mBlueUnits.size(), (int)Command::GOTO, ids);
		mInit = true;
	}

	mStartPos = mRedUnits->getCentreCurrent();

	for (unsigned int i=0; i < mCommands.size(); i++) {
		if (mBlueUnits[i]->isValidGroup()) {
			std::ostringstream oss;
			oss << "TargetGroup" << i << ">" << mBlueUnits[i]->getUnitIDs()[0];
			//update params
			updateParam(i, mBlueUnits[i]->getCentreCurrent(), oss.str(), mBlueUnits[i]->getUnitIDs());
			mEndPos[i] = mBlueUnits[i]->getCentreCurrent();
		} else {
			//delete parameter...
			disableCommand(i);
		}
	}
}

void TargetDirectionHypothesis::saveResults() {
	Hypothesis::saveResults();

	osg::Vec2 startPos(mStartPos.x(), mStartPos.y());

	for (unsigned int i=0; i < mCommands.size(); i++) {
		HypothesisResult* r = mResults[i]->getResult();
		r->setLabel(getName() + "TargetDir" + boost::lexical_cast<std::string>(i));
		r->addTarget(mBlueUnits[i]);
		std::vector<osg::Vec2> path;
		path.push_back(startPos);
		path.push_back(osg::Vec2(mEndPos[i].x(), mEndPos[i].y()));
		r->addPoints(path, HypothesisResult::WORLD);
	}
}
