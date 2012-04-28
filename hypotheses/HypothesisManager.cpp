/*
 * HypothesisManager.cpp
 *
 *  Created on: 27-Jan-2009
 *      Author: sbutler
 */

#include "HypothesisManager.h"
#include "HypothesisClient.h"
#include "Hypothesis.h"
#include "TargetHypothesis.h"
#include "TargetDirectionHypothesis.h"
#include "../packets.h"
#include "../mynetwork.h"
#include "../units/Unit.h"
#include "../models/inc/Targets.h"
#include "../models/inc/Observations.h"
#include "../gui/hammerqt.h"
#include "../eventlog.h"

#include <QString>
#include <QMessageBox>
#include "boost/lexical_cast.hpp"

dtCore::RefPtr<HypothesisManager> HypothesisManager::mManager = dtCore::RefPtr<HypothesisManager>(NULL);
//defaults
std::string HypothesisManager::mHost = "localhost";
int HypothesisManager::mPort = 4449;

HypothesisManager::HypothesisManager(Targets* targets, Observations* ob, hammerQt* gui):
	mTargets(targets),
	mObservations(ob),
	mGUI(gui),
	mInit(false)
{
	//setup a server listener on a new port
	if (theApp->getNetwork()->SetupServer(getPort())) {
		mManager = this;
	} else {
		if (theApp->isHeadless()) {
			exit(1);
		} else {
			QMessageBox::critical(NULL, QString("Server Error"),
					QString("Cannot open Hypothesis Manager port"), QMessageBox::Ok, QMessageBox::Ok);
		}
	}
	//connect(this, SIGNAL(hypothesisUpdate(std::vector<osg::Vec3>)), mGUI, SLOT(updateMapParams(std::vector<osg::Vec3>)));
}

HypothesisManager::~HypothesisManager() {
}

HypothesisManager* HypothesisManager::create(std::string host, int port, Targets* targets, Observations* ob, hammerQt* gui) {
	if (mManager.get() == NULL) {
		setPort(port);
		setHost(host);
		new HypothesisManager(targets, ob, gui);
	}
	return mManager.get();
}

HypothesisManager* HypothesisManager::getManager() {
	return mManager.get();
}
HypothesisManager* HypothesisManager::get() {
	return getManager();
}

/**
 * DEPRECATED: Hypotheses should be initialised by the Game subclass
 * Called by HTAppQt::launchHypotheses
 */
void HypothesisManager::initHypotheses() {
	std::vector<UnitGroup*> unitGroups = mTargets->getUnitGroups(RED);
	std::vector<UnitGroup*> targets = mTargets->getUnitGroups(BLUE);

	for (unsigned int i=0; i < unitGroups.size(); i++) {
		//set hypothesis
		//Hypothesis *h = new TargetHypothesis(targets, unitGroups[i]);
		Hypothesis *h = new TargetDirectionHypothesis(targets, unitGroups[i]);
		addHypothesis(h);
	}

	/*bool nextIndex() {
	  bool next = false;
	  for (int i = C; i >= 0; --i) {
	    if (index[i] + 1 < length(im[i])) {
	      index[i]++;
	      next = true;
	      break;
	    } else {
	      index[i] = 0;
	    }
	  }
	  return next;
	}

	int index[C];
	for (int i = 0; i < C; i++) {
	  index[i] = 0;
	}

	do {
	  H* h = new H(u);
	  for (int i = 0; i < C; i++) {
	    h->addIM(im[i][index[i]]);
	  }
	} while (nextIndex());*/

	mInit = true;
}

void HypothesisManager::addHypothesis(Hypothesis* h) {
	h->init();
	mHypotheses.push_back(h);
	mObservations->requestObservations(h);
	if (mGUI) mGUI->addHypothesis(h);
}

/**
 * Called by HTAppBase::RemoveUnit
 * Removes this unit from each hypothesis
 */
//void HypothesisManager::removeUnit(Unit* u) {

//}

/**
 * Called by HTAppQt::PreFrameGUI
 */
void HypothesisManager::update() {
	//we can't update if we have no hypotheses!
	if (!isInitialised()) return;
	//on every frame see if we have any idle clients
	std::vector<HypothesisClient*> clients = HypothesisClient::getIdleClients();
	for(std::vector<HypothesisClient*>::iterator it = clients.begin(); it != clients.end(); it++) {
		while (!mHypothesisQueue.empty()) {
			//select a hypothesis to continue on the idle client
			Hypothesis *h = mHypothesisQueue.front();
			if (!h->allCommandsSent() && h->isValid()) {
				h->startNextCommand(*it);
				break;
			} else {
				mHypothesisQueue.pop_front();
			}
		}
		if (mHypothesisQueue.empty()) {
			//we have an idle client and no hypotheses with starting observations
			//TODO start a hypothesis speculatively?
			//is there any point without an observation?
		}
	}

	//keep track of non-observer-based starting/actual state
	if (!mObservations->hasObservers() && EventLog::GetInstance().isTimeForNextLog()) {
		std::vector<Unit*> units = mTargets->getUnits();
		newState(units);
	}
	//TODO move this inside the loop below
	if (mGUI) mGUI->updateResults(mHypotheses);

	//log the performance of the hypotheses
	if (EventLog::GetInstance().isTimeForNextLog()) {
		for (unsigned int i=0; i < mHypotheses.size(); i++) {
			//calc centre of group
			osg::Vec3 pos = mHypotheses[i]->getObservationGroup()->getCentreCurrent();
			//calculate deviation
			float deviation = mHypotheses[i]->getDeviation(pos);
			//save to log file
			if (deviation != -1) {
				std::ostringstream oss;
				oss << "HYP  " << mHypotheses[i]->getID() << " " << deviation;
				EventLog::GetInstance().logEvent(oss.str());
			} else {
				std::ostringstream oss;
				oss << "HYP  " << mHypotheses[i]->getID() << " " << NAN;
				EventLog::GetInstance().logEvent(oss.str());
			}
		}
	}
}

Hypothesis *HypothesisManager::getNextHypothesis() {
	//TODO make this select the best hypothesis to execute for non-observer-based hypotheses
	if (mHypotheses.size() == 1 && mHypotheses.at(0)->isIdle()) {
		return mHypotheses.at(0).get();
	} else {
		return NULL;
	}
}

/**
 * Starts the execution of the hypothesis on the hypothesis client with the supplied state
 * The mHypothesisQueue is used to give priorities (on a first-come first-served basis)
 * to executing all the commands for the hypothesis on other available clients
 */
void HypothesisManager::startHypothesis(Hypothesis* h, HypothesisClient* c, std::vector<Unit*>& units, int duration) {
	if (h != NULL && c != NULL && c->isIdle()) {
		LOG_INFO("Found an idle client, starting hypothesis " + h->getName() + "...");
		EventLog::GetInstance().logEvent("HYPS " + boost::lexical_cast<std::string>(h->getID()));
		//get the blue units
		std::vector<Unit*> blueUnits = mTargets->getUnits(BLUE);
		std::copy(blueUnits.begin(), blueUnits.end(), std::back_inserter(units));
		//duration is decided by the observer (Observations::setTarget)
		//h->setDuration(duration);
		h->setDuration(duration);
		//TODO let the hypothesis decide on its time scale?
		h->setTimeScale(theApp->getSpeedup());
		//start the hypothesis
		h->start(c, units);
		mHypothesisQueue.push_back(h);
		//tell GUI we've started a new command set with new parameters
		emit hypothesisUpdate(h->getAllParams());
	} else {
		if (h==NULL) LOG_ERROR("Tried to start a hypothesis that doesn't exist")
		if (c==NULL) LOG_ERROR("Tried to start a hypothesis on a client that doesn't exist")
		else if (!c->isIdle()) LOG_ERROR("Tried to start a hypothesis on a client that is in use")
	}
}

/**
 * Called when we have a new observation for the completion of a hypothesis
 */
void HypothesisManager::finishHypothesis(Hypothesis* h, std::vector<Unit*>& units) {
	LOG_INFO("Finished observing hypothesis " + h->getName() + ", moving on...");
	h->saveActual(units);
}

/**
 * Called from the hypothesis after it has calculated the confidence
 */
void HypothesisManager::hypothesisFinished(Hypothesis* h) {
	LOG_INFO("Hypothesis " + h->getName() + " finished, with confidence "+boost::lexical_cast<std::string>(h->getConfidence()));

	//save in event log
	float deviation = h->getDeviation(h->getObservationGroup()->getCentreCurrent());
	std::ostringstream oss;
	oss << "CONF " << h->getID() << " " << h->getConfidence() << " " << ((deviation < 0) ? NAN : deviation);
	EventLog::GetInstance().logEvent(oss.str());

	//make sure hypothesis is still valid...
	//TODO do this validation more intelligently...
	if (h->isValid()) {
		mObservations->requestObservations(h);
	} else {
		LOG_INFO("Removing hypothesis " + h->getName())
		//remove? or put as low priority? or move to group-level prediction?
		//TODO replace hypothesis...
		//TODO remove from GUI
		mObservations->schedule();
	}
	//TODO see if this hypothesis can be merged with the ones nearby somehow

	//visualise finished hypothesis
	if (mGUI) mGUI->showHypothesisResults(h->getID(), h->getLatestResults());
	//TODO analyse finished hypothesis
}

/**
 * Start and/or finish hypotheses
 * Called from update when we don't have any observers
 */
void HypothesisManager::newState(std::vector<Unit*> &units) {
	LOG_DEBUG("Got a new state")
	mTimeStep++;
	//see if any of the clients are initialised and idle, therefore want to start a prediction
	std::vector<HypothesisClient*> clients = HypothesisClient::getIdleClients();
	for(std::vector<HypothesisClient*>::iterator it = clients.begin(); it != clients.end(); it++) {
		//select a hypothesis to run
		Hypothesis *h = getNextHypothesis();
		startHypothesis(h, *it, units, 5);
	}
	//see if any of the hypotheses need the world state to compare
	for (std::vector<dtCore::RefPtr<Hypothesis> >::iterator it = mHypotheses.begin(); it != mHypotheses.end(); it++) {
		//save actual positions of units when correct number of timesteps has elapsed
		int currentTime = getTimeStep();
		if (currentTime == (*it)->getFinishingTime()) {
			LOG_DEBUG("Saving actual")
			(*it)->saveActual(units);
		} else if ((*it)->isAwaitingActual() && currentTime > (*it)->getFinishingTime()) {
			LOG_ERROR("Time step (inexplicably) missed for receiving actual state! Discarding results...")
			(*it)->reset();
		}
	}
	//tell the GUI that we have new unit positions
	//emit unitsUpdate(units);
}

/**
 * Saves the unit prediction from a client and emits a signal when all units received
 * Called from MyNetwork::PreFrame when a new state is received from a client
 */
void HypothesisManager::newPrediction(int clientId, Unit* unit, int total) {
	//LOG_DEBUG("Got a prediction")
	HypothesisClient *client = HypothesisClient::getClient(clientId);
	if (client) {
		//save into a refptr
		//std::cout << unit->mUnitID << ": " << unit->m_Position << std::endl;
		mPredictedUnits[clientId].push_back(unit);
		//if got all predictions
		if (++(mPredictedUnitsCount[clientId]) == total) {
			LOG_DEBUG("Got all predictions")
			mPredictedUnitsCount[clientId] = 0;
			//save them
			for (std::vector<dtCore::RefPtr<Hypothesis> >::iterator it = mHypotheses.begin(); it != mHypotheses.end(); it++) {
				if ((*it)->isAwaitingPredictions(clientId)) {
					//saves predictions
					(*it)->savePredictions(client, mPredictedUnits[clientId]);

					//we've finished executing on the client
					client->setIdle(true);
					break;
				}
			}
			mPredictedUnits[clientId].clear();
			//show in GUI (scroll table)
			//emit updateResults(client);
		}
	} else {
		LOG_ERROR("Prediction from unknown client")
	}
}

double HypothesisManager::getTimeStep() {
	if (mObservations->hasObservers()) {
		return theApp->getSimTime();
	} //else
	return mTimeStep;
}
