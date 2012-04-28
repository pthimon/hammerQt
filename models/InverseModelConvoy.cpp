#include "InverseModelConvoy.h"

#include "InverseModelFormation.h"
#include "InverseModelPath.h"
#include "../units/UnitFiring.h"
#include "../units/UnitGroup.h"
#include "inc/Targets.h"

InverseModelConvoy::InverseModelConvoy(std::vector<Unit*>& units): mUnits(units)
{
	mGroup = new UnitGroup(RED);
	mGroup->addUnits(units);
	allocate();
}

InverseModelConvoy::~InverseModelConvoy()
{
}

void InverseModelConvoy::update() {
	// TODO when arrived, go to next city
}

void InverseModelConvoy::removeUnit(Unit* unit, bool clear) {
	InverseModel::removeUnit(unit, clear);
	
	//find out if this unit is part of our subset
	std::vector<Unit*>::iterator it;
	for (it = mUnits.begin(); it != mUnits.end(); ) {
		if (*it == unit) {
			mUnits.erase(it);
		} else {
			it++;
		}
	}
}

//path to nearest defensive position
void InverseModelConvoy::allocate() {
	osg::Vec3 mCityPosition = theApp->getTargets()->getNearestCityPosition(mGroup->getCentre());
	std::vector<Unit*>::iterator it;
	for (it = mUnits.begin(); it != mUnits.end(); it++) {
		UnitFiring* u = dynamic_cast<UnitFiring*>(*it);
		if (u) {
			u->setAutoFire(false);
		}
		InverseModelPath::createPath(*it,mCityPosition);
	}
	theApp->addInverseModel(new InverseModelFormation(mUnits, InverseModelFormation::COLUMN, true, true));
}
