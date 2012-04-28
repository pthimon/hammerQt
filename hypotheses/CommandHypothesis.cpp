/*
 * TargetDirectionHypothesis.cpp
 *
 *  Created on: 18 Jan 2010
 *      Author: sbutler
 */

#include "CommandHypothesis.h"

#include "HypothesisResults.h"
#include "../Command.h"
#include <boost/lexical_cast.hpp>

CommandHypothesis::CommandHypothesis(std::vector<UnitGroup*> blueUnits, UnitGroup* myUnits, Command::CommandType command, std::string name):
	Hypothesis(blueUnits, myUnits)
{
	//just one command - attack nearest group
	std::vector<int> myUnitIDs = mRedUnits->getUnitIDs();
	std::vector<int> myTargetUnits; //not specified
	osg::Vec3 targetPos(0,0,0); //not specified
	addCommand((int)command, myUnitIDs, targetPos, name, myTargetUnits);
}

CommandHypothesis::~CommandHypothesis() {
}
