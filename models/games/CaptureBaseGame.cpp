/*
 * EvasionGame.cpp
 *
 *  Created on: 26 Jan 2010
 *      Author: sbutler
 */

#include "CaptureBaseGame.h"

#include "../../common.h"
#include "../../HTAppBase.h"
#include "../InverseModelAttack.h"
#include "../InverseModelRetreat.h"
#include "../InverseModelDefend.h"
#include "../InverseModelConvoy.h"
#include "../InverseModelPath.h"
#include "../../units/Unit.h"
#include "../../units/UnitGroup.h"
#include "../inc/Targets.h"
#include "../../hypotheses/Hypothesis.h"
#include "../../hypotheses/CommandHypothesis.h"
#include "../../hypotheses/HypothesisManager.h"
#include "../../eventlog.h"

#include <boost/lexical_cast.hpp>
#include <algorithm>

CaptureBaseGame::CaptureBaseGame() : Game() {
	theApp->setSides(BLUE,RED);

	mDefendingGroups = theApp->getTargets()->getUnitGroups(RED);
	mAttackingGroups = theApp->getTargets()->getUnitGroups(BLUE);
}

CaptureBaseGame::~CaptureBaseGame() {
}

void CaptureBaseGame::initHypotheses() {
	//initialise hypotheses for defenders
	Command::CommandType hypotheses[] = {Command::MANOEUVRE_ATTACK, Command::MANOEUVRE_RETREAT, Command::MANOEUVRE_DEFEND, Command::MANOEUVRE_CONVOY };
	std::string hypothesesNames[] = {"Attack", "Retreat", "Defend", "Convoy"};

	for (std::vector<UnitGroup*>::iterator it = mDefendingGroups.begin(); it != mDefendingGroups.end(); it++) {
		for (unsigned int i=0; i < 4; i++) {
			Hypothesis *h = new CommandHypothesis(mAttackingGroups, *it, hypotheses[i], hypothesesNames[i]);
			//mManager->addHypothesis(h);
			mDefenderHypotheses[*it] = h;
		}
	}

	LOG_DEBUG("Initialised Capture Base Game.")
}

void CaptureBaseGame::initIMs() {
	//initialise IM for defenders
	int models[] = {1,2,3,4};
	//get random permutation
	int p = rand() % 24;
	for (int i=0; i < p; i++) {
		std::next_permutation(models,models+4);
	}
	std::cout << mDefendingGroups.size() << std::endl;
	unsigned int j=0;
	for (std::vector<UnitGroup*>::iterator it = mDefendingGroups.begin(); it != mDefendingGroups.end(); it++) {
		std::vector<Unit*> units = (*it)->getUnits();
		InverseModel* im = NULL;
		switch (models[j]) {
		case 1:
			im = new InverseModelAttack(units);
			LOG_DEBUG("Assigned Attack Inverse Model")
			break;
		case 2:
			im = new InverseModelRetreat(units);
			LOG_DEBUG("Assigned Retreat Inverse Model")
			break;
		case 3:
			im = new InverseModelDefend(units);
			LOG_DEBUG("Assigned Defend Inverse Model")
			break;
		case 4:
			im = new InverseModelConvoy(units);
			LOG_DEBUG("Assigned Convoy Inverse Model")
			break;
		}
		mDefenderModels[*it] = im;
		if (!isReplay()) theApp->addInverseModel(im);
		j++;
	}

	//initialise IM for attackers
	for (std::vector<UnitGroup*>::iterator it = mAttackingGroups.begin(); it != mAttackingGroups.end(); it++) {
		osg::Vec3 goal = theApp->mapPointToTerrain(theApp->getTargets()->getNearestGoalPosition((*it)->getCentre()));
		std::vector<Unit*> units = (*it)->getUnits();
		for (std::vector<Unit*>::iterator uit = units.begin(); uit != units.end(); uit++) {
			InverseModelPath::createPath(*uit, goal);
		}
	}

	LOG_DEBUG("Initialised Inverse Models.")
}

void CaptureBaseGame::updateGame() {

}

