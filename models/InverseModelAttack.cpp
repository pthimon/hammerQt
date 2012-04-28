#include "InverseModelAttack.h"
#include "../units/UnitFiring.h"

InverseModelAttack::InverseModelAttack(std::vector<Unit*>& units): mUnits(units)
{
	allocate();
}

InverseModelAttack::~InverseModelAttack()
{
}

void InverseModelAttack::update() {
	//TODO periodically reallocate?
	allocate();
	std::vector<std::pair<Unit*,Unit*> >::iterator it;
	for (it = mAllocation.begin(); it != mAllocation.end(); it++) {
		//make sure target is available
		if (it->second) {
			//set target position to be furthest away along vector with line of sight
			UnitFiring* unit = dynamic_cast<UnitFiring*>(it->first);
			if (unit) {
				osg::Vec3 unitPos = unit->getProjectileLaunchPos();
				osg::Vec3 enemyPos = it->second->getPosition();
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
				it->first->setGoalPosition(targetPos);
			}
		}
	}
}

void InverseModelAttack::removeUnit(Unit* unit, bool clear) {
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
void InverseModelAttack::allocate() {
	//TODO proper allocation
	mAllocation.clear();
	std::vector<Unit*>::iterator it;
	for (it = mUnits.begin(); it != mUnits.end(); it++) {
		mAllocation.push_back(std::pair<Unit*,Unit*>(*it, getNearestEnemyUnitTo(*it)));
	}
}
