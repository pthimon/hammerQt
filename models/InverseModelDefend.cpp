#include "InverseModelDefend.h"

#include "../units/UnitFiring.h"
#include "../units/UnitGroup.h"
#include "inc/Targets.h"
#include "InverseModelPath.h"

InverseModelDefend::InverseModelDefend(std::vector<Unit*>& units): mUnits(units)
{
	mGroup = new UnitGroup(RED);
	mGroup->addUnits(units);
	allocate();
}

InverseModelDefend::~InverseModelDefend()
{
}

void InverseModelDefend::update() {
	//when arrived, start firing
	unsigned int i=0;
	std::vector<Unit*>::iterator it;
	for (it = mUnits.begin(); it != mUnits.end(); it++) {
		if (!mArrived[i]) {
			float dist = (mDefensivePosition - (*it)->getPosition()).length2();
			if (dist < 5) {
				mArrived[i] = 1;
				UnitFiring* u = dynamic_cast<UnitFiring*>(*it);
				if (u) {
					u->setAutoFire(true);
				}
			}
		}
		i++;
	}
}

void InverseModelDefend::removeUnit(Unit* unit, bool clear) {
	InverseModel::removeUnit(unit, clear);
	
	//find out if this unit is part of our subset
	unsigned int i=0;
	std::vector<Unit*>::iterator it;
	for (it = mUnits.begin(); it != mUnits.end(); ) {
		if (*it == unit) {
			mUnits.erase(it);
			mArrived.erase(mArrived.begin()+i);
		} else {
			it++;
			i++;
		}
	}
}

//path to nearest defensive position
void InverseModelDefend::allocate() {
	osg::Vec3 mDefensivePosition = theApp->getTargets()->getNearestDefensivePosition(mGroup->getCentre());
	std::vector<Unit*>::iterator it;
	for (it = mUnits.begin(); it != mUnits.end(); it++) {
		UnitFiring* u = dynamic_cast<UnitFiring*>(*it);
		if (u) {
			u->setAutoFire(false);
		}
		InverseModelPath::createPath(*it,mDefensivePosition);
		mArrived.push_back(0);
	}
}
