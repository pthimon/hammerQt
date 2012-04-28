/*
 * DefaultGame.cpp
 *
 *  Created on: 15 Feb 2010
 *      Author: sbutler
 */

#include "DefaultGame.h"

#include "../../common.h"
#include "../../hypotheses/HypothesisManager.h"
#include "../../hypotheses/TargetDirectionHypothesis.h"
#include "../../hypotheses/TargetHypothesis.h"
#include "../../units/UnitGroup.h"
#include "../../models/inc/Targets.h"

#include <vector>

DefaultGame::DefaultGame() {
	// TODO Auto-generated constructor stub

}

DefaultGame::~DefaultGame() {
	// TODO Auto-generated destructor stub
}

void DefaultGame::initHypotheses() {
	std::vector<UnitGroup*> unitGroups = theApp->getTargets()->getUnitGroups(RED);
	std::vector<UnitGroup*> targets = theApp->getTargets()->getUnitGroups(BLUE);

	for (unsigned int i=0; i < unitGroups.size(); i++) {
		//set hypothesis
		Hypothesis *h = new TargetHypothesis(targets, unitGroups[i]);
		//Hypothesis *h = new TargetDirectionHypothesis(targets, unitGroups[i]);
		mManager->addHypothesis(h);
	}
}

void DefaultGame::updateGame() {
	//manage groups splitting/merging here...
}
