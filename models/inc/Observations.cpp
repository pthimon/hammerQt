/*
 * Observations.cpp
 *
 *  Created on: 07-Apr-2009
 *      Author: sbutler
 */

#include "Observations.h"

#include "../../common.h"
#include "../../models/inc/Targets.h"
#include "../../models/inc/InverseModel.h"
#include "../../units/Unit.h"
#include "../../units/UnitObserver.h"
#include "../../units/UnitGroup.h"
#include "../../hypotheses/Hypothesis.h"
#include "../../hypotheses/HypothesisManager.h"
#include "../../hypotheses/HypothesisClient.h"
#include "../../eventlog.h"
#include <Eigen/Core>
#include <boost/lexical_cast.hpp>

/**
 * Observation class
 */
Observations::Observations():
	mNumObservableGroups(0),
	mNumObservers(0),
	mScheduleFlag(false)
{

}

Observations::~Observations() {
}

void Observations::addObserver(UnitObserver* o) {
	mObservers.push_back(o);
	mScheduleFlag = true;
	if (o->getType() != GLOBAL) mNumObservers++;
}

void Observations::removeObserver(UnitObserver* o) {
	mObservers.erase(std::remove(mObservers.begin(),mObservers.end(),o), mObservers.end());
}

bool Observations::hasObservers() {
	return mObservers.size();
}

void Observations::requestObservations(Hypothesis* h) {
	//get observation groups
	//std::vector<dtCore::RefPtr<UnitGroup> > groups = h->getUnitGroups();
	//append to mPendingRequests
	//std::copy(groups.begin(),groups.end(),std::back_inserter(mPendingRequests));

	mPendingRequests.push_back(h);

	mNumObservableGroups = theApp->getTargets()->getUnitGroups(RED).size();

	mScheduleFlag = true;
}

/*void Observations::requestObservation(Unit* unit) {
	mPendingRequests.push_back(new UnitGroup(unit));
	mScheduleFlag = true;
}
void Observations::requestObservation(int unitId) {
	mPendingRequests.push_back(new UnitGroup(theApp->getUnitById(unitId)));
	mScheduleFlag = true;
}

void Observations::requestObservations(std::vector<Unit*> units) {
	for (unsigned int i = 0; i < units.size(); i++) {
		mPendingRequests.push_back(new UnitGroup(units[i]));
	}
	mScheduleFlag = true;
}
void Observations::requestObservations(std::vector<int> units) {
	for (unsigned int i = 0; i < units.size(); i++) {
		mPendingRequests.push_back(new UnitGroup(theApp->getUnitById(units[i])));
	}
	mScheduleFlag = true;
}
void Observations::requestObservations(UnitSide side) {
	mPendingRequests = theApp->getTargets()->groupObservedUnits(side);
	mScheduleFlag = true;
}
*/

/*std::deque<dtCore::RefPtr<ScheduleItem> > Observations::nearestNeighbour(osg::Vec3 start, std::vector<Unit*> requests) {
	std::deque<dtCore::RefPtr<ScheduleItem> > schedule;
	//nearest neighbour from start position
	float dMax = std::numeric_limits<float>::max();
	std::vector<Unit*>::iterator itMax;
	for (std::vector<Unit*>::iterator it = requests.begin(); it != requests.end(); it++) {
		float d = ((*it)->getPosition() - start).length();
		if (d < dMax) {
			dMax = d;
			itMax = it;
		}
	}
	schedule.push_back(new ScheduleItem(*itMax));
	requests.erase(itMax);
	//nearest neighbour
	while (!requests.empty()) {
		dMax = std::numeric_limits<float>::max();
		osg::Vec3 pos = schedule.back()->getTarget();
		for (std::vector<Unit*>::iterator it = requests.begin(); it != requests.end(); it++) {
			float d = ((*it)->getPosition() - pos).length();
			if (d < dMax) {
				dMax = d;
				itMax = it;
			}
		}
		schedule.push_back(new ScheduleItem(*itMax));
		requests.erase(itMax);
	}
	return schedule;
}*/

dtCore::RefPtr<UnitGroup> Observations::nearestNeighbour(osg::Vec3 start, std::vector<dtCore::RefPtr<UnitGroup> >& requests) {
	float dMax = std::numeric_limits<float>::max();
	std::vector<dtCore::RefPtr<UnitGroup> >::iterator itMax;
	for (std::vector<dtCore::RefPtr<UnitGroup> >::iterator it = requests.begin(); it != requests.end(); it++) {
		float d = ((*it)->getCentre() - start).length();
		if (d < dMax) {
			dMax = d;
			itMax = it;
		}
	}
	return *itMax;
}

osg::Vec3 Observations::getObservationPoint(osg::Vec3 startPos, float viewRadius, osg::Vec3 targetPos, float targetRadius) {
	osg::Vec3 targetDir = targetPos - startPos;
	float targetDist = targetDir.length();

	if ((targetDist + targetRadius) <= viewRadius) {
		//we can already see you!
		return startPos;
	} else {
		//find the point that is viewRadius away from the target pos in the direction of travel
		targetDir.normalize();
		osg::Vec3 obsPos = targetPos + (-targetDir * (viewRadius - targetRadius));
		return obsPos;
	}
}


void Observations::setTarget(UnitObserver* o) {
	UnitObserverType type = o->getType();

	if (mPendingRequests.empty() && type != GLOBAL) {
		//nothing to do
		o->setIdle();
		//EventLog::GetInstance().logEvent("IDLE "+o->getName());
		return;
	}

	LOG_INFO("Setting new target for observer "+o->getName());

	//if we have the same number of observers as hypotheses, then don't move them from their target group after it is set
	if (mNumObservers >= mNumObservableGroups && o->hasTargetUnits()) {
		//see if we're requesting this hypothesis again
		std::deque<Hypothesis*>::iterator hit = std::find(mPendingRequests.begin(), mPendingRequests.end(), o->getHypothesis());
		if (hit != mPendingRequests.end()) {
			o->setTarget(o->getHypothesis(), o->getTargetGroup(), (float)o->getAltitude(), o->getTargetDuration());
			mPendingRequests.erase(hit);
		}
		//EventLog::GetInstance().logEvent("FULL "+o->getName());
		return;
	}

	switch (type) {
		case GLOBAL: {
			//move to the centre of the observed groups
			/*std::vector<dtCore::RefPtr<UnitGroup> > groups = theApp->getTargets()->groupUnits(5000, RED);
			if (groups.size() > 0) {
				UnitGroup* g = groups[0].get();
				osg::Vec3 centre = g->getCentre();
				float radius = g->getRadius();
				centre.z() += radius;
				o->setTarget(centre, 1);
			}*/
		}
		break;
		case SEQUENTIAL: {
			//nearest neighbour (+ priorities) from start position, and wait 5 seconds
			float dMax = theApp->getTerrain()->GetHorizontalSize() * sqrt(2); //size of map
			std::deque<Hypothesis*>::iterator it;
			float dMin = std::numeric_limits<float>::max();
			dtCore::RefPtr<UnitGroup> group;
			Hypothesis* hypothesis = NULL;
			for (it = mPendingRequests.begin(); it != mPendingRequests.end(); it++) {
				dtCore::RefPtr<UnitGroup> g = (*it)->getObservationGroup();
				float d = (g->getCentre() - o->getPosition()).length();
				//normalise
				d /= dMax;
				//add in hypothesis priorities
				float p = d + (*it)->getPriority();
				//
				LOG_INFO("Hypothesis group "+boost::lexical_cast<std::string>((*it)->getName())+": Priority "+boost::lexical_cast<std::string>(p)+" ("+boost::lexical_cast<std::string>(d)+" + "+boost::lexical_cast<std::string>((*it)->getPriority()))
				//std::cout << (*it)->getName() << ":d" << d <<",t"<<(*it)->getPriority()<<",p"<<p<<std::endl;
				if (p < dMin) {
					dMin = p;
					group = g;
					hypothesis = *it;
				}
			}
			//dtCore::RefPtr<UnitGroup> u = nearestNeighbour(o->getPosition(), mPendingRequests);
			//o->setTarget(u.get(), 500, 2);
			//TODO set duration and altitude properly
			o->setTarget(hypothesis, group.get(), 500, theApp->getObservationDuration());
			mPendingRequests.erase(std::find(mPendingRequests.begin(), mPendingRequests.end(), hypothesis));
		}
		break;
		case ROUND_ROBIN: {
			if (!mPendingRequests.empty()) {
				//EventLog::GetInstance().logEvent("OBST "+mPendingRequests.front()->getName());
				o->setTarget(mPendingRequests.front(), mPendingRequests.front()->getObservationGroup().get(), 500, theApp->getObservationDuration());
				mPendingRequests.pop_front();
			}
			/*std::queue<Hypothesis*>::iterator it = mPendingRequests.begin();
			if (it != mPendingRequests.end()) {
				mPendingRequests.erase(it);
			}*/
		}
		break;
		case THREAT: {
			//calculate resultant threat of observing each target

			//decide what target each observee is going to...
			//conf > 0.5. conf decays (or variance increases) without observations.
			//target certainty... take closest if conf < 0.5 or if variance = global variance

			//err, lets just take closest for now...
			osg::Vec3 oPos = theApp->mapPointToTerrain(o->getPosition());
			std::deque<Hypothesis*>::iterator it;
			dtCore::RefPtr<UnitGroup> group;
			Hypothesis* hypothesis = NULL;
			UnitGroup* predictedGroup = NULL;
			UnitGroup* closestGroup = NULL;
			float confidence = 0;
			float minLeeway = std::numeric_limits<float>::max();
			for (it = mPendingRequests.begin(); it != mPendingRequests.end(); it++) {
				dtCore::RefPtr<UnitGroup> g = (*it)->getObservationGroup();
				osg::Vec3 gPos = g->getCentreCurrent();
				//float gRadius = g->getRadius();
				std::vector<dtCore::RefPtr<UnitGroup> > targetGroups = (*it)->getTargetGroups();
				dtCore::RefPtr<UnitGroup> targetGroup;
				float dMin = std::numeric_limits<float>::max();
				//find closest target group for this observation group
				for (std::vector<dtCore::RefPtr<UnitGroup> >::iterator it2 = targetGroups.begin(); it2 != targetGroups.end(); it2++ ) {
					float d = (gPos - (*it2)->getCentreCurrent()).length();
					//TODO fix: subtract radii from centre to centre distance
					//d = d - gRadius - (*it2)->getRadius();
					if (d < 0) d = 0;
					if (d < dMin) {
						dMin = d;
						targetGroup = *it2;
					}
				}
				//calc threat (time = dMin / speed)
				//if targetgroup is the same as the highest confidence (>0.5) group then assume running away
				UnitGroup* predictedTarget = (*it)->getTargetGroup();
				float conf = (*it)->getConfidence();
				if (predictedTarget != NULL && predictedTarget == targetGroup.get() && conf > 0.5) {
					//dMin = v2*ds/(v2-v1)
					dMin = (g->getMaxSpeed() * dMin) / (g->getMaxSpeed() - targetGroup->getMaxSpeed());
				}
				float attackTime = dMin / g->getMaxSpeed();

				//find time for observer to travel to observation group
				float travelTime = (gPos - oPos).length() / o->getMaxSpeed();

				//calc leeway
				float leeway = attackTime - travelTime;

				std::ostringstream oss;
				oss << "OBSL " << o->getName() << " " << (*it)->getName() << " " << attackTime << " " << travelTime << " " << leeway;
				EventLog::GetInstance().logEvent(oss.str());

				//take lowest (non-negative) leeway
				if (leeway > 0 && leeway < minLeeway) {
					minLeeway = leeway;
					group = g;
					hypothesis = *it;
					predictedGroup = predictedTarget;
					closestGroup = targetGroup.get();
					confidence = conf;
				}
			}
			//if hypothesis has high confidence, but not at min leeway target, then see if more important target to view
			//i.e., make better use of our time as min leeway isn't very likely
			if (hypothesis && confidence > 0.5 && closestGroup != predictedGroup) {
				float minTime = minLeeway;
				for (it = mPendingRequests.begin(); it != mPendingRequests.end(); it++) {
					float conf = (*it)->getConfidence();
					if (conf <= 0) {
						dtCore::RefPtr<UnitGroup> g = (*it)->getObservationGroup();
						osg::Vec3 gPos = g->getCentreCurrent();
						float travelTime = (gPos - oPos).length() / o->getMaxSpeed();
						//time to get there, observe, and get back
						float totalTime = (travelTime * 2) + (*it)->getSimTime();
						if (totalTime < minTime) {
							hypothesis = *it;
							group = g;
						}
					}
				}
			}
			//we didn't find any group to look at (probably they were all predicted (inf))
			if (hypothesis == NULL) {
				//find the hypothesis that has longest time since last obs.
				double maxTime = 0;
				for (it = mPendingRequests.begin(); it != mPendingRequests.end(); it++) {
					double t = (*it)->getTimeDiff();
					if (t >= maxTime) {
						maxTime = t;
						hypothesis = *it;
					}
				}
				group = hypothesis->getObservationGroup();
			}

			o->setTarget(hypothesis, group.get(), 500, theApp->getObservationDuration());
			mPendingRequests.erase(std::find(mPendingRequests.begin(), mPendingRequests.end(), hypothesis));

			LOG_DEBUG("Setting target to "+hypothesis->getName())
			//not sure if one target will hog the observer...
		}
		break;
		/*case SCANNING: {
			//find nearest neighbour
			dtCore::RefPtr<UnitGroup> u = nearestNeighbour(o->getPosition(), mPendingRequests);
			//find point where it comes into view
			osg::Vec3 obsPos = getObservationPoint(o->getGroundPosition(), o->getViewRadius(), u->getCentre(), u->getRadius());

			float totalTime = 2;
			float timeLeft = (totalTime / 2) - 0.5; //allowing time to turn around and return
			while (timeLeft > 0) {
				//find next neighbour
				dtCore::RefPtr<UnitGroup> u2 = nearestNeighbour(obsPos, mPendingRequests);
				osg::Vec3 obsPos2 = getObservationPoint(obsPos, o->getViewRadius(), u2->getCentre(), u2->getRadius());
				float nextDist = (obsPos - obsPos2).length();

				//see if we can get to next nearest neighbour and back within 2 seconds
				float travelTime = getTimeToTravel(nextDist);
				if (travelTime < timeLeft) {
					//add
					o->addTarget(u.get(), 500, 2);
					mPendingRequests.erase(std::find(mPendingRequests.begin(), mPendingRequests.end(), u));
				}
				timeLeft -= travelTime;
			}
			//add the targets in reverse order to get back...

			o->setTarget(u.get(), 500, 2);
			mPendingRequests.erase(std::find(mPendingRequests.begin(), mPendingRequests.end(), u));
		}
		break;*/
		/*case GROUP: {
			//get group
			dtCore::RefPtr<UnitGroup> u = nearestNeighbour(o->getPosition(), mPendingRequests);
			UnitGroup* group = theApp->getTargets()->groupObservedUnits(RED,u.get(),250);
			//set target position
			o->setTarget(group, 300, 2);
			//remove observed units
			std::vector<Unit*> units = group->getUnits();
			for (unsigned int i=0; i<units.size(); i++) {
				for (std::vector<dtCore::RefPtr<UnitGroup> >::iterator it = mPendingRequests.begin(); it != mPendingRequests.end(); it++) {
					if ((*it)->isInGroup(units[i])) {
						mPendingRequests.erase(it);
						break;
					}
				}
			}
		}
		break;*/
		case FOLLOWING: {
			//if leader, find nearest neighbour
			/*dtCore::RefPtr<UnitGroup> u = nearestNeighbour(o->getPosition(), mPendingRequests);
			o->setTarget(itMax->get(), 500, 0);
			mPendingRequests.erase(std::find(mPendingRequests.begin(), mPendingRequests.end(), u));
			*///if follower get to leaders position 2 seconds later
		}
		break;
	}

}

void Observations::schedule() {
	//set targets for idle observers when we have some new requests
	/*std::deque<Hypothesis*>::iterator it;
	for (it = mPendingRequests.begin(); it != mPendingRequests.end(); it++) {
		EventLog::GetInstance().logEvent("REQ "+(*it)->getName());
	}*/
	for (unsigned int i=0; i < mObservers.size(); i++) {
		if (mObservers[i]->isIdle()) {
			EventLog::GetInstance().logEvent("OBS "+mObservers[i]->getName());
			setTarget(mObservers[i]);
		}
	}
//TODO do an optimised static schedule as a basis for timings
//wont work if the units move though

	/*for (unsigned int i=0; i < mObservers.size(); i++) {
		mSchedule[i].clear();

		UnitObserver::Type type = mObservers[i]->getType();

		if (mPendingRequests.empty() && type != UnitObserver::GLOBAL) {
			continue;
		}

		switch (type) {
		case UnitObserver::GLOBAL:
		{
			std::vector<dtCore::RefPtr<UnitGroup> > groups = theApp->getTargets()->groupUnits(5000, RED);
			if (groups.size() > 0) {
				UnitGroup* g = groups[0].get();
				osg::Vec3 centre = g->getCentre();
				float radius = g->getRadius();
				centre.z() += radius;
				//position in centre of group at an altitude to cover everything
				//osg::Vec3 p = theApp->mapPointToTerrain(osg::Vec3(0,0,0));
				//mSchedule[i].push_back(new ScheduleItem(p + osg::Vec3(0,0,7000), 0));
				mSchedule[i].push_back(new ScheduleItem(centre));
			}
		}
			break;
		case UnitObserver::STATIC:
		case UnitObserver::SCANNING:
		case UnitObserver::FOLLOWING:
			//run the nearest neighbour scheduling

				std::vector<Unit*> requests;
				for (std::map<Unit*, int>::iterator it = mPendingRequests.begin(); it != mPendingRequests.end(); it++) {
					requests.push_back(it->first);
				}
				mSchedule[i] = nearestNeighbour(mObservers[0]->getPosition(), requests);
		}
	}*/
	mScheduleFlag = false;
}

/**
 * Called by HTAppBase::PreFrame()
 */
void Observations::update(float /*delta*/) {
	//update all the friendly blue team to have perfect observed information
	std::vector<Unit*> units = theApp->getTargets()->getUnits(BLUE);
	for (unsigned int i=0; i < units.size(); i++) {
		units[i]->mObservedPosition = units[i]->getPosition();
		units[i]->mObservedPositionTime = theApp->getSimTime();
		units[i]->mObservedPositionVariance = 0;
	}

	if (mScheduleFlag) schedule();

	//loop through all observers and update the positions of observed enemy (red) units
	for (unsigned int i=0; i < mObservers.size(); i++) {
		//where observer is
		osg::Vec3 pos = mObservers[i]->getPosition();
		float radius = mObservers[i]->getViewRadius();
		float resolution = mObservers[i]->getViewResolution();

		float noise = sqrt(mObservers[i]->getViewVariance())/10;
		// add some noise to observer position, use a normal distribution
		pos.x() += theApp->getRandomGaussian(noise);
		pos.y() += theApp->getRandomGaussian(noise);

		//see if we happen to observe anything
		std::vector<Unit*> enemies = theApp->getTargets()->getUnits(RED);
		for (unsigned int j=0; j < enemies.size(); j++) {
			float dist = (enemies[j]->getPosition()-pos).length();
			//make sure we can see them and we can improve the variance of the observation
			if (dist < mObservers[i]->getViewLength()) {
				enemies[j]->setObservedPosition(pos,radius,resolution);
			}
		}

		//see if the observer has arrived at the target
		if (!mObservers[i]->isIdle()) {
			if (mObservers[i]->hasArrived()) {
				//make sure the hypothesis we're executing is valid
				if (mObservers[i]->getHypothesis() && !mObservers[i]->getHypothesis()->isValid()) {
					mObservers[i]->setIdle();
					schedule();
				}
				//alert HypothesisManager
				if (mObservers[i]->hasTargetUnits()) {
					HypothesisClient* client = HypothesisClient::getIdleClient();
					//see if we have a client to execute on
					if (client != NULL) {
						std::vector<Unit*> units = mObservers[i]->getTargetUnits();
						HypothesisManager::get()->startHypothesis(mObservers[i]->getHypothesis(), client, units, mObservers[i]->getTargetDuration());
						mObservers[i]->startObservation();
					}
				} else {
					mObservers[i]->startObservation();
				}
			}

			//if we have finished observing update the observers target
			if (mObservers[i]->hasObservedTarget()) {
				//alert HypothesisManager
				if (!mObservers[i]->isIdle() && mObservers[i]->hasTargetUnits()) {
					std::vector<Unit*> units = mObservers[i]->getTargetUnits();
					HypothesisManager::get()->finishHypothesis(mObservers[i]->getHypothesis(), units);
				}
				//set new target, if there is one, otherwise set idle
				//setTarget(mObservers[i]); //commented: wait until conf calculated before setting new target
				mObservers[i]->setIdle();
			}
		}
	}
	//update groups
	theApp->getTargets()->updateObservations(RED);
}
