/*
 * EvasionGame.cpp
 *
 *  Created on: 26 Jan 2010
 *      Author: sbutler
 */

#include "DirectionGame.h"

#include "../../common.h"
#include "../../HTAppBase.h"
#include "../InverseModelTrack.h"
#include "../../units/Unit.h"
#include "../inc/Targets.h"
#include "../../hypotheses/Hypothesis.h"
#include "../../hypotheses/TargetDirectionHypothesis.h"
#include "../../hypotheses/TargetHypothesis.h"
#include "../../hypotheses/DirectionHypothesis.h"
#include "../../hypotheses/HypothesisManager.h"
#include "../../eventlog.h"
#include "TargetReplayCommand.h"

#include <boost/lexical_cast.hpp>

DirectionGame::DirectionGame() : Game(),
	mNumLevels(5),
	mNumSegments(6)
{
}

DirectionGame::~DirectionGame() {
}

void DirectionGame::initHypotheses() {
	//initialise hypotheses for prey
	std::vector<UnitGroup*> predatorGroups = theApp->getTargets()->getUnitGroups(RED);
	std::vector<UnitGroup*> preyGroups = theApp->getTargets()->getUnitGroups(BLUE);

	for (std::vector<UnitGroup*>::iterator it = predatorGroups.begin(); it != predatorGroups.end(); it++) {
		Hypothesis *h = new DirectionHypothesis(preyGroups, *it, mNumLevels, mNumSegments);
		//Hypothesis *h = new TargetHypothesis(preyGroups, *it);
		mManager->addHypothesis(h);
		std::vector<Unit*> predators = (*it)->getUnits();
		for (unsigned int i=0; i < predators.size(); i++) {
			mPredatorHypotheses[predators[i]] = h;
		}
	}

	LOG_DEBUG("Initialised Direction Game.")
}

void DirectionGame::initIMs() {
	//get alive units
	std::vector<Unit*> predators = theApp->getTargets()->getUnits(RED);

	//initialise IM for predators
	for (std::vector<Unit*>::iterator it = predators.begin(); it != predators.end(); it++) {
		InverseModelTrack* im = new InverseModelTrack(*it, true);
		mPredatorModels[*it] = im;
		if (!isReplay()) theApp->addInverseModel(im);
	}
}

GameReplayCommand* DirectionGame::parseReplayCommand(std::ifstream& ifs, double timeStamp, std::string command) {
	if (command == "TCHG") {
		std::string predator, prey;
		ifs >> predator >> prey;
		Unit* predatorUnit = theApp->getUnitByName(predator);
		Unit* preyUnit = theApp->getUnitByName(prey);
		return new TargetReplayCommand(timeStamp, predatorUnit, preyUnit);
	} //TODO replay should also deal with unit death, otherwise inconsistencies appear...
	return NULL;
}

void DirectionGame::applyReplayCommand(GameReplayCommand* c) {
	TargetReplayCommand* ec = dynamic_cast<TargetReplayCommand*>(c);
	if (ec) {
		if (ec->getPredator() != NULL) {
			LOG_DEBUG("Applying replay target to "+ec->getPredator()->getName())
			mPredatorModels[ec->getPredator()]->setTarget(ec->getPrey());
		}
	}
}

void DirectionGame::updateGame() {
	//get alive units
	std::vector<Unit*> predators = theApp->getTargets()->getUnits(RED);
	std::vector<Unit*> preys = theApp->getTargets()->getUnits(BLUE);

	std::map<Unit*, std::vector<Unit*> > preyPredators;
	for (std::vector<Unit*>::iterator it = preys.begin(); it != preys.end(); it++) {
		//initialise map with empty vectors, so every prey appears when iterating through the map
		preyPredators[*it] = std::vector<Unit*>();
	}

	//update predators
	for (std::vector<Unit*>::iterator it = predators.begin(); it != predators.end(); it++) {
		//predators cannot die:
		(*it)->setHealth(100);
		//see if we are near enough to eat our target
		if (!isReplay() && mPredatorModels[*it] && mPredatorModels[*it]->getTarget() != NULL) {
			Unit* p = mPredatorModels[*it]->getTarget();
			if (((*it)->getPosition() - p->getPosition()).length() < 10) {
				//see if prediction was correct
				std::vector<int> units = mPredatorHypotheses[*it]->getTargetUnits();
				float conf = mPredatorHypotheses[*it]->getConfidence();
				for (unsigned int i=0; i < units.size(); i++) {
					EventLog::GetInstance().logEvent("PRED UNIT "+(theApp->getUnitById(units[i]))->getName());
				}
				if (conf > 0 && std::find(units.begin(), units.end(), p->getUnitID()) != units.end()) {
					EventLog::GetInstance().logEvent("PRED TRUE "+(*it)->getName()+" "+p->getName());
				} else {
					EventLog::GetInstance().logEvent("PRED FALSE "+(*it)->getName()+" "+p->getName());
				}
				std::ostringstream oss;
				oss << "PRDC " << conf << " ";
				std::copy(units.begin(), units.end(), std::ostream_iterator<int>(oss, " "));
				EventLog::GetInstance().logEvent(oss.str());
				//eat
				p->setHealth(0);
				//target dies & new target selected when we next get around to PreFrame
			}
		}

		//if (!isReplay() && ((mPredatorModels[*it] && mPredatorModels[*it]->getTarget() == NULL) || (rand() % 10000 < 1))) {
		if (!isReplay() && ((mPredatorModels[*it] && mPredatorModels[*it]->getTarget() == NULL))) {
			//select new target & IM if dead
			selectTarget(*it, preys);
		}

		//find their most likely target, update prey
		/*if (mPredatorHypotheses[*it]->getConfidence() > 0.5) {
			//get old targets
			std::vector<int> oldPreyIDs = mPredatorTargets[*it];
			bool targetChange = false;
			std::ostringstream oss;
			//get current targets
			std::vector<int> preyIDs = mPredatorHypotheses[*it]->getTargetUnits();
			for(unsigned int i=0; i < preyIDs.size(); i++) {
				//see if the target has changed: if this prey wasn't in the old prey list
				if (!targetChange && std::find(oldPreyIDs.begin(), oldPreyIDs.end(), preyIDs[i]) == oldPreyIDs.end()) {
					targetChange = true;
				}
				//update the predators attacking this prey
				Unit* prey = theApp->getUnitById(preyIDs[i]);
				preyPredators[prey].push_back(*it);
				oss << prey->getName() << " ";
			}
			if (targetChange) {
				//save change
				EventLog::GetInstance().logEvent("PCHG "+(*it)->getName()+" "+boost::lexical_cast<std::string>(preyIDs.size())+" "+oss.str());
				mPredatorTargets[*it] = preyIDs;
			}
		}*/
	}

	/*for (std::map<Unit*, std::vector<Unit*> >::iterator it = preyPredators.begin(); it != preyPredators.end(); it++) {
		Unit* prey = it->first;
		osg::Vec3 preyPos = prey->getPosition();
		if (it->second.size() == 0) {
			//if we have no predators attacking then stop
			prey->setGoalPosition(prey->getPosition(), false);
		} else if (it->second.size() == 1) {
			//if we have one predator attacking, go in opposite direction
			osg::Vec3 preyDir = preyPos - it->second[0]->getPosition();
			prey->setGoalPosition(preyPos + preyDir, false);
		} else {
			//if we have more than one predator attacking, find best direction to go in
			std::vector<float> weights;
			for (int i=0; i < 8; i++) {
				weights.push_back(0);
			}
			//for each predator attacking prey calculate the weight for a particular direction
			for (unsigned int i=0; i < it->second.size(); i++) {
				for (unsigned int j=0; j < weights.size(); j++) {
					weights[j] += 1 / ((preyPos + mOffsets[j]) - it->second[i]->getPosition()).length();
				}
			}
			//go in the direction with least weight
			float minWeight = std::numeric_limits<float>::max();
			int dir = -1;
			for (unsigned int j=0; j < weights.size(); j++) {
				if (weights[j] < minWeight) {
					minWeight = weights[j];
					dir = j;
				}
			}
			prey->setGoalPosition(preyPos + (mOffsets[dir] * 100), false);
		}
	}*/
}

void DirectionGame::selectTarget(Unit* predator, std::vector<Unit*>& prey) {
	//find distance to all prey
	std::multimap<float, Unit*> distances;
	osg::Vec3 predPos = predator->getPosition();
	for (std::vector<Unit*>::iterator it = prey.begin(); it != prey.end(); it++) {
		float dist = ((*it)->getPosition() - predPos).length();
		distances.insert(std::pair<float, Unit*>(dist, *it));
	}
	//randomly select from top three
	if (distances.size() > 0) {
		int r = rand() % ((distances.size() < 3) ? distances.size() : 3);
		int i = 0;
		for (std::multimap<float, Unit*>::iterator it = distances.begin(); it != distances.end(); it++) {
			if (i == r) {
				//update IM
				mPredatorModels[predator]->setTarget(it->second);
				return;
			}
			i++;
		}
	}
}


