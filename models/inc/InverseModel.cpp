#include "InverseModel.h"

InverseModel::InverseModel()
{
	std::set<Unit*> u = theApp->GetAllUnits();
	//std::copy(u.begin(), u.end(), mAllUnits.begin());
	mAllUnits.clear();
	std::set<Unit*>::iterator it;
	for(it=u.begin(); it!=u.end(); it++) {
		mAllUnits.push_back(*it);
	}
}

InverseModel::~InverseModel()
{
}

void InverseModel::addUnit(Unit* unit) {
	//each inverse model maintains a list of all the units in the scene
	mAllUnits.push_back(unit);
}
void InverseModel::removeUnit(Unit *unit, bool clear) {
	//called when a unit is destroyed
	mAllUnits.erase(std::remove(mAllUnits.begin(),mAllUnits.end(),unit), mAllUnits.end());
}

Unit* InverseModel::getNearestEnemyUnitTo(Unit *unit) {
	//find closest enemy unit
	osg::Vec3 pos = unit->getPosition();
	float dist = INFINITY;
	Unit *closest = NULL;
	std::vector<Unit*>::iterator it;
	for (it = mAllUnits.begin(); it != mAllUnits.end(); it++) {
		if (unit->getSide() != (*it)->getSide()) {
			double d = ((*it)->getPosition() - pos).length();
			if (d < dist) {
				dist = d;
				closest = *it;
			}
		}
	}
	return closest;
}

