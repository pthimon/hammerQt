#include "InverseModelRetreat.h"
#include "../units/UnitFiring.h"

InverseModelRetreat::InverseModelRetreat(std::vector<Unit*>& units): mUnits(units)
{
	allocate();
}

InverseModelRetreat::~InverseModelRetreat()
{
}

void InverseModelRetreat::update() {
	//TODO periodically reallocate?
	allocate();
	std::vector<std::pair<Unit*,Unit*> >::iterator it;
	for (it = mAllocation.begin(); it != mAllocation.end(); it++) {
		//make sure target is available
		if (it->second) {
			//set target position to be furthest away along vector without line of sight
			UnitFiring* unit = dynamic_cast<UnitFiring*>(it->first);
			if (unit) {
				osg::Vec3 unitPos = unit->getProjectileLaunchPos();
				osg::Vec3 enemyPos = it->second->getPosition();
				UnitFiring* enemy = dynamic_cast<UnitFiring*>(it->second);
				if (enemy)
					enemyPos = enemy->getProjectileLaunchPos();
				osg::Vec3 enemyDir = enemyPos - unitPos;
				float len = enemyDir.length();
				if (enemy)
					len = enemy->getProjectileRange()+10;
				enemyDir.normalize();
				//sample along the direction vector for positions without line of sight, default: get out of range
				osg::Vec3 targetPos = unitPos - (enemyDir * len);
				for (unsigned int i=0; i <= len; i++) {
					targetPos = unitPos - (enemyDir * i);
					//if there is no line of sight then stop. (with some buffer in the z-axis)
					if (!theApp->getTerrain()->IsClearLineOfSight(osg::Vec3(targetPos[0], targetPos[1], targetPos[2]+5), enemyPos)) {
						break;
					}
				}
				it->first->setGoalPosition(targetPos);
			}
		}
	}
}

void InverseModelRetreat::removeUnit(Unit* unit, bool clear) {
	InverseModel::removeUnit(unit, clear);
	
	//find out if this unit is part of our subset
	std::vector<Unit*>::iterator r = std::remove(mUnits.begin(), mUnits.end(), unit);
	
	if (r != mUnits.end()) {
		mUnits.erase(r, mUnits.end());
	}
	
	if (!clear) {
		//TODO reallocate targets only if the removed unit is in mUnits or a target
		allocate();
	}
}

//attack the nearest enemy unit
void InverseModelRetreat::allocate() {
	//TODO proper allocation
	mAllocation.clear();
	std::vector<Unit*>::iterator it;
	for (it = mUnits.begin(); it != mUnits.end(); it++) {
		mAllocation.push_back(std::pair<Unit*,Unit*>(*it, getNearestEnemyUnitTo(*it)));
	}
}
