/*
 * UnitGroup.cpp
 *
 *  Created on: 29-Apr-2009
 *      Author: sbutler
 */

#include "UnitGroup.h"

#include "Unit.h"
#include "UnitFiring.h"
#include "../models/inc/Targets.h"
#include <algorithm>

UnitGroup::UnitGroup(UnitSide side, UnitGroupType type, float range):
	mSide(side), mType(type), mRange(range) {

}
UnitGroup::UnitGroup(Unit* unit) {
	mSide = unit->getSide();
	mType = OBSERVED_GROUP;
	addUnit(unit);
}

UnitGroup::~UnitGroup() {

}

void UnitGroup::addUnits(std::vector<Unit*> units) {
	for (unsigned int i=0; i < units.size(); i++) {
		addUnit(units[i]);
	}
}
void UnitGroup::addUnit(Unit* unit) {
	if (unit != NULL) {
		mUnits.push_back(unit);
		updateBB(unit);
	}
}

void UnitGroup::update() {
	//clear mBB
	mBB = osg::BoundingBox();
	//add all units to the bb
	for (unsigned int i=0; i < mUnits.size(); i++) {
		if (theApp->unitExists(mUnits[i])) {
			updateBB(mUnits[i]);
		}
	}
}

void UnitGroup::updateBB(Unit* unit) {
	if (mType == GROUP) {
		mBB.expandBy(unit->getMaxBoundingBox());
	} else if (mType == OBSERVED_GROUP) {
		mBB.expandBy(unit->getObservedBoundingBox());
	}
}

void UnitGroup::removeDeadUnits() {
	//remove any dead units
	for (std::vector<Unit*>::iterator it = mUnits.begin(); it != mUnits.end();) {
		if (!theApp->unitExists(*it)) {
			mUnits.erase(it);
		} else {
			it++;
		}
	}
}

std::vector<Unit*> UnitGroup::getUnits() {
	//remove any dead units
	removeDeadUnits();

	return mUnits;
}
std::vector<int> UnitGroup::getUnitIDs() {
	std::vector<int> u;
	for (unsigned int i=0; i < mUnits.size(); i++) {
		u.push_back(mUnits[i]->getUnitID());
	}
	return u;
}

bool UnitGroup::isInGroup(Unit* u) {
	return (std::find(mUnits.begin(),mUnits.end(), u) != mUnits.end());
}

bool UnitGroup::isValidGroup() {
	removeDeadUnits();

	if (mUnits.size() == 0) {
		return false;
	}
	//make sure this is still a coherent group
	if (mRange > 0) {
		//see if the remaining units are within range FIXME
		std::vector<Unit*> unitsIn = mUnits;
		theApp->getTargets()->findGroup(mUnits[0], unitsIn, mRange);
		if (!unitsIn.empty()) {
			//some units missing!
			return false;
		}
	}
	return true;
}

osg::Vec3 UnitGroup::getCentre() {
	return theApp->mapPointToTerrain(mBB.center());
}

/**
 * getCentre only returns the centre of the bounding box, which is only updated on update();
 */
osg::Vec3 UnitGroup::getCentreCurrent() {
	removeDeadUnits();

	osg::Vec3 pos;
	for (unsigned int j=0; j < mUnits.size(); j++) {
		pos += mUnits[j]->getPosition();
	}
	pos /= mUnits.size();
	return pos;
}

osg::Vec3 UnitGroup::getLOSCentre() {
	osg::Vec3 centre = mBB.center();
	centre.z() = mBB.zMax();
	return centre;
}

osg::Vec3 UnitGroup::getClosestUnitPosition(osg::Vec3 pos) {
	removeDeadUnits();

	osg::Vec3 posClose;
	double maxDist = std::numeric_limits<double>::max();
	double maxHeight = -std::numeric_limits<double>::max();
	for (unsigned int j=0; j < mUnits.size(); j++) {
		osg::Vec3 pos2 = mUnits[j]->getPosition();
		double dist = (pos2-pos).length2();
		if (dist < maxDist) {
			posClose = pos2;
			maxDist = dist;
		}
		if (pos2.z() > maxHeight) {
			maxHeight = pos2.z();
		}
	}
	posClose.z() = maxHeight;
	return posClose;
}

float UnitGroup::getRadius() {
	return mBB.radius();
}
float UnitGroup::getRadiusX() {
	return 0.5 * (mBB.xMax() - mBB.xMin());
}
float UnitGroup::getRadiusY() {
	return 0.5 * (mBB.yMax() - mBB.yMin());
}

osg::BoundingBox UnitGroup::getBoundingBox() {
	return mBB;
}

UnitSide UnitGroup::getSide() {
	return mSide;
}

/**
 * Update all units to go to a new group centre
 */
void UnitGroup::setGoalPosition(osg::Vec3 pos) {
	osg::Vec3 centre = getCentre();
	for (std::vector<Unit*>::iterator it = mUnits.begin(); it != mUnits.end(); it++) {
		osg::Vec3 offset = (*it)->getPosition() - centre;
		(*it)->setGoalPosition(pos + offset);
	}
}

/**
 * Update all units to go to an additional group centre
 */
void UnitGroup::addGoalPosition(osg::Vec3 pos) {
	osg::Vec3 centre = getCentre();
	for (std::vector<Unit*>::iterator it = mUnits.begin(); it != mUnits.end(); it++) {
		osg::Vec3 offset = (*it)->getPosition() - centre;
		(*it)->addGoalPosition(pos + offset);
	}
}

/**
 * Max speed of group is the slowest speed of each of the units
 * (assuming group stays together)
 */
float UnitGroup::getMaxSpeed() {
	float minSpeed = std::numeric_limits<float>::max();
	for (std::vector<Unit*>::iterator it = mUnits.begin(); it != mUnits.end(); it++) {
		float speed = (*it)->getMaxSpeed();
		if (speed < minSpeed) {
			minSpeed = speed;
		}
	}
	return minSpeed;
}

float UnitGroup::getRange() {
	float rangeMax = 0;
	for (std::vector<Unit*>::iterator it = mUnits.begin(); it != mUnits.end(); it++) {
		UnitFiring* u = dynamic_cast<UnitFiring*>(*it);
		if (u) {
			float range = u->getProjectileRange2();
			if (range > rangeMax) {
				rangeMax = range;
			}
		}
	}
	return rangeMax + getRadius();
}
