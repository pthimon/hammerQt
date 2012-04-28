/*
 * InverseModelTrack.cpp
 *
 *  Created on: 26 Jan 2010
 *      Author: sbutler
 */

#include "InverseModelTrack.h"

#include "../units/UnitFiring.h"
#include "../eventlog.h"

InverseModelTrack::InverseModelTrack(Unit* predator, bool path) :
	mPredator(predator),
	mPrey(NULL),
	mDoPath(path)
{
}

InverseModelTrack::~InverseModelTrack() {
}

void InverseModelTrack::update() {
	if (mPredator && mPrey) {

		/*UnitFiring* unit = dynamic_cast<UnitFiring*>(mPredator);
		if (unit) {
			osg::Vec3 unitPos = unit->getProjectileLaunchPos();
			osg::Vec3 enemyPos = mPrey->getPosition();
			osg::Vec3 enemyDir = enemyPos - unitPos;
			float len = enemyDir.length();
			enemyDir.normalize();
			//sample along the direction vector for positions with line of sight
			unitPos[2] -= 1; //add some buffer to the z-axis to make sure there really is line of sight
			osg::Vec3 targetPos = enemyPos;
			for (unsigned int i=0; i < len; i+=2) {
				targetPos = unitPos + (enemyDir * i);
				if (unit->isTargetInRange(enemyPos) && theApp->getTerrain()->IsClearLineOfSight(targetPos, enemyPos)) {
					break;
				}
			}
			mPredator->setGoalPosition(targetPos);
		}*/

		/*if (mDoPath) {
			setPath(mPredator, mPrey->getPosition());
		} else {
			mPredator->setGoalPosition(mPrey->getPosition(), false);
		}*/

		//TODO if prey moved update path periodically
	}
}

void InverseModelTrack::setTarget(Unit* u) {
	mPrey = u;
	//setPath(mPredator, mPrey->getPosition());
	if (u) {
		EventLog::GetInstance().logEvent("TCHG "+mPredator->getName()+" "+mPrey->getName());
	}
	if (mDoPath) {
		setPath(mPredator, mPrey->getPosition());
	} else {
		mPredator->setGoalPosition(mPrey->getPosition(), false);
	}
}

void InverseModelTrack::setPath(Unit *unit, osg::Vec3 goal) {
	//add new point(s)
	std::vector< void* > path;
	float totalCost;
	osg::Vec3 pos;

	//do search
	pos = unit->getPosition();
	int result = theApp->getTerrainMap()->solve(pos, goal, &path, &totalCost, unit);

	if (result == micropather::MicroPather::SOLVED) {
		PathInterpolation unitPath;
		//points
		for (unsigned int i=1; i<path.size(); i++) {
			osg::Vec3 pos = theApp->getTerrainMap()->stateToVec3(path[i]);
			unitPath.addWaypoint(pos);
		}
		unit->setGoalPath(unitPath, false);
	} else {
		std::cout << "WARN: No path found" << std::endl;
	}
}

Unit* InverseModelTrack::getTarget() {
	return mPrey;
}

void InverseModelTrack::removeUnit(Unit* unit, bool clear) {
	InverseModel::removeUnit(unit, clear);

	if (unit == mPredator) {
		mPredator = NULL;
	}

	if (unit == mPrey) {
		mPrey = NULL;
	}
}

bool InverseModelTrack::valid() {
	if (mPredator == NULL) {
		return false;
	}
	return true;
}
