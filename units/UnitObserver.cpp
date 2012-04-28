/*
 * UnitObserver.cpp
 *
 *  Created on: 07-Apr-2009
 *      Author: sbutler
 */

#include <dtCore/transform.h>

#include "UnitObserver.h"

#include "../common.h"
#include "UnitGroup.h"

UnitObserver::UnitObserver(UnitObserverType type, const std::string& name, osg::Vec3 pos, int id):
	Unit(name, pos, 0, NONE, false, false, true, -1, id ),
	mType(type)
{
	mSpeed = 150.0;
	mAltitude = 0.0;
	mTurnRate = 200.0;
	mStoppingDist = 30.0;
	mSlowingDist = 60.0;
	mFOV = M_PI/4;
	mResolution = 50;
	mTargetArrivedTime = 0;
	mTargetDuration = 0;
	mTargetUnit = NULL;
	mTargetGroup = NULL;
	mMinAltitude = 100;
	mArrivedFlag = false;
	mIdle = true;
}

UnitObserver::~UnitObserver() {

}

void UnitObserver::update() {
	Unit::update();

	if (!mArrivedFlag && mTargetArrivedTime < 0 && isAtTarget()) {
		mArrivedFlag = true;
	}
}

/**
 * Rotate the soldier to face the heading
 * Rate of turn defined by mTurningRate
 */
void UnitObserver::turnTo(double angle) {
	double turn_factor = 0;

	if (angle < 0) {
		turn_factor = 1;
	} else if (angle > 0) {
		turn_factor = -1;
	}

	//set rotation
	double heading = getHeading();
	heading += turn_factor * mTurnRate * theApp->getFrameTime();
	if (!isnan(heading)) setHeading(heading);
}

osg::Vec3 UnitObserver::getPosition() {
	//get position (Unit::getPosition is not accurate in the z dimension)
	dtCore::Transform current_tr;
	dtCore::Transformable* next_object =
		dynamic_cast<dtCore::Transformable*>(GetChild(0));
	next_object->GetTransform(current_tr);
	osg::Vec3 pos = current_tr.GetTranslation();
	return pos;
}
osg::Vec3 UnitObserver::getGroundPosition() {
	return theApp->mapPointToTerrain(getPosition());
}

bool UnitObserver::setGoalPosition(osg::Vec3 goalPos, bool marker) {
	double h;
	theApp->GetTerrainHeight(goalPos.x(),goalPos.y(), h);

	//if goal position is on the ground then add desired altitude
	if ((goalPos.z() - h) < mMinAltitude) {
		if ((goalPos.z()-h) < 0) {
			//if goal position is under the terrain then map it to the ground first
			goalPos = theApp->mapPointToTerrain(goalPos);
		}
		goalPos.z() += mAltitude;
	}

	return Unit::setGoalPosition(goalPos, marker);
}

/**
 * Update the observers position
 * Called by Unit::update
 * (not actually physics-based movement ;-)
 */
void UnitObserver::updatePhysics() {
	osg::Vec3 pos = getPosition();
	osg::Vec3 goalPos = getGoalPosition();

	float dist = getDistanceToGoal();
	float speed = 0;
	if (dist > mSlowingDist) {
		//move to goal
		speed = mSpeed;
		double angle = getAngleToGoal();
		turnTo(angle);
	} else if (dist > mStoppingDist) {
		speed = (mSpeed / mSlowingDist) * dist;
		double angle = getAngleToGoal();
		turnTo(angle);
	}

	//move
	pos += (getHeadingVector() * (speed * theApp->getFrameTime()));

	//stay targetAltitude above the terrain
	float targetZ = goalPos.z();
	double h;
	theApp->GetTerrainHeight(goalPos.x(),goalPos.y(), h);
	float targetAltitude = targetZ - h;
	theApp->GetTerrainHeight(pos.x(),pos.y(), h);
	float target = h + targetAltitude;
	float vspeed = mSpeed;
	float vdist = fabs(target - pos.z());
	if (vdist < mSpeed) {
		vspeed = vdist;
	}
	float hvec = -1;
	if ((target - pos.z()) > 0) {
		hvec = 1;
	}
	pos.z() += (hvec * vspeed * theApp->getFrameTime());
	setPosition(pos);
}

void UnitObserver::LoadMeshes() {
	std::ostringstream ss;
	ss << GetName() << "0";

	dtCore::RefPtr<dtCore::Object> object = new dtCore::Object(ss.str());

	if (!object->LoadFile("StaticMeshes/uh-1n.ive")) {
		LOG_ERROR("Cannot find remote model: StaticMeshes/uh-1n.ive");
		return;
	}
	object->EnableDynamics(false);

	AddChild(object.get());
}

float UnitObserver::getTimeToTravel(float dist) {
	return dist / mSpeed;
}

double UnitObserver::getAltitude() {
	osg::Vec3 pos = getPosition();
	double h;
	theApp->GetTerrainHeight(pos.x(),pos.y(),h);
	return pos.z()-h;
}
void UnitObserver::setAltitude(double a) {
	mAltitude = a;
}

double UnitObserver::getViewLength() {
	//distance from target to observer
	//hyp = altitude / sin(FOV)
	return getAltitude() / sin(mFOV);
}

double UnitObserver::getViewRadius() {
	//hyp = altitude * tan(FOV)
	return getAltitude() * tan(mFOV);
}

float UnitObserver::getViewVariance() {
	float res = getViewRadius() / mResolution;
	return res*res;
}

float UnitObserver::getViewResolution() {
	return mResolution;
}

double UnitObserver::getObservedPositionVariance() {
	double r = getViewRadius();
	return r*r;
}

UnitObserverType UnitObserver::getType() {
	return mType;
}

/*void UnitObserver::setTarget(Unit* u, float altitude, float duration) {
	setAltitude(altitude);
	mTargetUnit = u;
	mTargetDuration = duration;
	mTargetArrivedTime = -1;
	mArrivedFlag = false;
}*/

void UnitObserver::setTarget(Hypothesis* h, UnitGroup* u, float minAltitude, float duration) {
	setAltitude(minAltitude);
	mMinAltitude = minAltitude;
	mTargetGroup = u;
	mTargetDuration = duration;
	mTargetArrivedTime = -1;
	mArrivedFlag = false;
	mHypothesis = h;
	mIdle = false;
}

void UnitObserver::setTarget(osg::Vec3 pos, float duration) {
	setGoalPosition(pos);
	mTargetDuration = duration;
	mTargetArrivedTime = -1;
	mArrivedFlag = false;
	mIdle = false;
}

/**
 * If observer has no target to observe then set it to be idle
 */
void UnitObserver::setIdle() {
	mTargetArrivedTime = -1;
	mArrivedFlag = false;
	mIdle = true;
	LOG_INFO("Observer set to IDLE")
}

bool UnitObserver::isIdle() {
	return mIdle;
}

float UnitObserver::getTargetDuration() {
	return mTargetDuration;
}

bool UnitObserver::hasArrived() {
	return mArrivedFlag;
}

void UnitObserver::startObservation() {
	mArrivedFlag = false;
	mTargetArrivedTime = theApp->getSimTime();
	//TODO update target position/altitude based on observed targets max speed and duration
}

bool UnitObserver::hasObservedTarget() {
	return (mTargetArrivedTime >= 0 && theApp->getSimTime() > (mTargetArrivedTime + mTargetDuration));
}

/**
 * Protected function.
 */
bool UnitObserver::isAtTarget() {
	//see if we've arrived at the target
	if (mTargetUnit != NULL) {
		//target is a unit, first make sure it is alive
		if (theApp->unitExists(mTargetUnit)) {
			osg::Vec3 target = mTargetUnit->getPosition();
			//always head to its latest position
			setGoalPosition(target);

			//just go within viewing range of the target
			float dist = (target-getPosition()).length();
			if (dist < getViewLength()) {
				return true;
			}
		} else {
			//unit is dead, so go to the next target
			mTargetDuration = 0;
			mTargetArrivedTime = 0;
			return true;
		}
	} else if (mTargetGroup.valid()) {
		//find the centre of the group
		mTargetGroup->update();
		osg::Vec3 target = mTargetGroup->getCentre();
		if (mMinAltitude < mTargetGroup->getRadius()) {
			setAltitude(mTargetGroup->getRadius());
		}
		setGoalPosition(target);

		//just go within viewing range of target + radius
		float dist = (target-getGroundPosition()).length() + mTargetGroup->getRadius();
		if (dist < getViewRadius()) {
			return true;
		}
	} else {
		//target is a position
		osg::Vec3 target = getGoalPosition();

		//go within mStoppingDist of the target
		float dist = (target-getPosition()).length();
		if (dist < mStoppingDist) {
			return true;
		}
	}
	return false;
}

bool UnitObserver::hasTargetUnits() {
	return ((mTargetUnit != NULL) || mTargetGroup.valid());
}

std::vector<Unit*> UnitObserver::getTargetUnits() {
	std::vector<Unit*> u;
	if (mTargetUnit != NULL) {
		//target is a unit, first make sure it is alive
		if (theApp->unitExists(mTargetUnit)) {
			u.push_back(mTargetUnit);
		}
	} else if (mTargetGroup.valid()) {
		u = mTargetGroup->getUnits();
	}
	return u;
}

UnitGroup* UnitObserver::getTargetGroup() {
	if (mTargetGroup.valid()) {
		return mTargetGroup.get();
	}
	return NULL;
}

Hypothesis* UnitObserver::getHypothesis() {
	return mHypothesis;
}

float UnitObserver::getMaxSpeed() {
	return mSpeed;
}

