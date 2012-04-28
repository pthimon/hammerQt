/*
 * Targets.cpp
 *
 *  Created on: 03-Apr-2009
 *      Author: sbutler
 */

#include "Targets.h"

#include "../../terrain/terrain.h"
#include "../../units/Unit.h"
#include "../../units/UnitGroup.h"
#include <algorithm>

Targets::Targets(Terrain *terrain, std::set< dtCore::RefPtr<Unit> > *units, std::vector<dtCore::RefPtr<UnitGroup> > *groups):
	mTerrain(terrain),
	mUnits(units),
	mGroups(groups)
{
	//find the defensive positions in the terrain (only needs to be done once)
	long unsigned int defensiveLayerMask = 1l << mTerrain->GetLayerIndex("defensive");
	//find the cities in the terrain (only needs to be done once)
	long unsigned int cityLayerMask = 1l << mTerrain->GetLayerIndex("city");
	//find the goal positions in the terrain (only needs to be done once)
	long unsigned int goalLayerMask = 1l << mTerrain->GetLayerIndex("goal");

	float xmax = mTerrain->GetHeightField()->GetNumColumns()-1;
	float ymax = mTerrain->GetHeightField()->GetNumRows()-1;
	for (int x = 0; x < xmax; x++) {
		for (int y = 0; y < ymax; y++) {
			if (mTerrain->GetLayerBitsAt(x, y) & defensiveLayerMask) {
				float wx, wy;
				mTerrain->ProjectCoordinates(x,y,xmax,ymax,wx,wy);
				float wz = mTerrain->GetHeight(wx,wy);
				mDefensivePositions.push_back(osg::Vec3(wx,wy,wz));
			}
			/*if (mTerrain->GetLayerBitsAt(x, y) & cityLayerMask) {
				float wx, wy;
				mTerrain->ProjectCoordinates(x,y,xmax,ymax,wx,wy);
				float wz = mTerrain->GetHeight(wx,wy);
				mCityPositions.push_back(osg::Vec3(wx,wy,wz));
			}
			if (mTerrain->GetLayerBitsAt(x, y) & goalLayerMask) {
				float wx, wy;
				mTerrain->ProjectCoordinates(x,y,xmax,ymax,wx,wy);
				float wz = mTerrain->GetHeight(wx,wy);
				mGoalPositions.push_back(osg::Vec3(wx,wy,wz));
			}*/
		}
	}
	mCityPositions.push_back(theApp->mapPointToTerrain(osg::Vec3(-3200,2800,0)));
	mCityPositions.push_back(theApp->mapPointToTerrain(osg::Vec3(2800,-3100,0)));

	mGoalPositions.push_back(theApp->mapPointToTerrain(osg::Vec3(2000,250,0)));
}

Targets::~Targets() {
}

/**
 * Called by HTAppBase::PreFrame to group units
 */
void Targets::update() {
	*mGroups = groupUnits(150);
}
/**
 * Called from Observations::update()
 */
void Targets::updateObservations(UnitSide side) {
	mObservedGroups = groupObservedUnits(side);
}

/**
 * Groups units within a range
 */
std::vector<dtCore::RefPtr<UnitGroup> > Targets::groupUnits(float range, UnitSide side) {
	std::vector<dtCore::RefPtr<UnitGroup> > groups;
	std::vector<Unit*> units = getUnits(side);
	while (!units.empty()) {
		//starting unit
		std::vector<Unit*>::iterator vit = units.begin();
		Unit* start = *vit;
		dtCore::RefPtr<UnitGroup> g = new UnitGroup(start->getSide(), UnitGroup::GROUP, range);
		units.erase(vit);
		//find any other units that are within range
		g->addUnits(findGroup(start, units, range));
		groups.push_back(g);
	}
	return groups;
}

/**
 * Groups units within their observed position variance
 * Called from Observations::update()
 */
std::vector<dtCore::RefPtr<UnitGroup> > Targets::groupObservedUnits(UnitSide side) {
	std::vector<dtCore::RefPtr<UnitGroup> > groups;
	std::vector<Unit*> units = getObservedUnits(side);
	while (!units.empty()) {
		//starting unit
		std::vector<Unit*>::iterator vit = units.begin();
		Unit* start = *vit;
		dtCore::RefPtr<UnitGroup> g = new UnitGroup(start->getSide(), UnitGroup::OBSERVED_GROUP);
		units.erase(vit);
		//find any other units that are within the observed position variance
		g->addUnits(findGroup(start, units, 0));
		groups.push_back(g);
	}
	return groups;
}

/**
 * Groups observed units within a range of a starting group
 */
UnitGroup* Targets::groupObservedUnits(UnitSide side, UnitGroup* start, float range) {
	std::vector<Unit*> units = getObservedUnits(side);
	//starting unit
	Unit* startUnit = start->getUnits()[0];
	UnitGroup* g = new UnitGroup(start->getSide(), UnitGroup::OBSERVED_GROUP);
	units.erase(std::find(units.begin(), units.end(), startUnit));
	//find any other units that are within range
	g->addUnits(findGroup(startUnit, units, range));

	return g;
}

std::vector<Unit*> Targets::getUnits(UnitSide side, osg::Vec3 start, float radius) {
	//get all the actual units
	std::vector<Unit*> units;
	std::set< dtCore::RefPtr<Unit> >::iterator it;
	for (it = mUnits->begin(); it != mUnits->end(); it++) {
		if ((side == ALL || (*it)->getSide() == side) && (radius == 0 || (radius > 0 && ((*it)->getPosition()-start).length() <= radius) )) {
			units.push_back(it->get());
		}
	}

	return units;
}

std::vector<Unit*> Targets::getUnitSnapshots(UnitSide side, osg::Vec3 start, float radius) {
	//get all the actual units
	std::vector<Unit*> units;
	std::set< dtCore::RefPtr<Unit> >::iterator it;
	for (it = mUnits->begin(); it != mUnits->end(); it++) {
		if ((side == ALL || (*it)->getSide() == side) && (radius == 0 || (radius > 0 && ((*it)->getPosition()-start).length() <= radius) )) {
			//make a clone of the active unit
			units.push_back((*it)->clone());
		}
	}

	return units;
}

std::vector<Unit*> Targets::getObservedUnits(UnitSide side) {
	//get all the actual units
	std::vector<Unit*> units;
	std::set< dtCore::RefPtr<Unit> >::iterator it;
	for (it = mUnits->begin(); it != mUnits->end(); it++) {
		if ((side == ALL || (*it)->getSide() == side) && (*it)->isObserved()) {
			units.push_back(it->get());
		}
	}

	return units;
}

std::vector<UnitGroup*> Targets::getUnitGroups(UnitSide side) {
	std::vector<UnitGroup*> groups;
	for (std::vector<dtCore::RefPtr<UnitGroup> >::iterator it = mGroups->begin(); it != mGroups->end(); it++) {
		if (side == ALL || (*it)->getSide() == side || (side == ACTIVE && (*it)->getSide() > side)) {
			groups.push_back(it->get());
		}
	}
	return groups;
}

std::vector<UnitGroup*> Targets::getObservedUnitGroups(UnitSide side) {
	std::vector<UnitGroup*> groups;
	for (std::vector<dtCore::RefPtr<UnitGroup> >::iterator it = mObservedGroups.begin(); it != mObservedGroups.end(); it++) {
		if (side == ALL || (*it)->getSide() == side || (side == ACTIVE && (*it)->getSide() > side)) {
			groups.push_back(it->get());
		}
	}
	return groups;
}

std::vector<int> Targets::getUnitIDs(UnitSide side, osg::Vec3 start, float radius) {
	std::vector<int> units;
	std::set< dtCore::RefPtr<Unit> >::iterator it;
	for (it = mUnits->begin(); it != mUnits->end(); it++) {
		if ((side == ALL || (*it)->getSide() == side) && (radius == 0 || (radius > 0 && ((*it)->getPosition()-start).length() <= radius) )) {
			units.push_back((*it)->getUnitID());
		}
	}

	return units;
}

std::vector<osg::Vec3> Targets::getDefensivePositions() {
	return mDefensivePositions;
}

osg::Vec3 Targets::getNearestDefensivePosition(osg::Vec3 start) {
	std::vector<osg::Vec3>::iterator it;
	float nearest_dist = std::numeric_limits<float>::max();
	osg::Vec3 nearest;
	for (it = mDefensivePositions.begin(); it != mDefensivePositions.end(); it++) {
		float dist = ((*it)-start).length();
		if (dist < nearest_dist) {
			nearest_dist = dist;
			nearest = *it;
		}
	}
	return nearest;
}

osg::Vec3 Targets::getNearestDefensivePositionFrom(Unit* unit) {
	//TODO calc visibility from enemies
	return getNearestDefensivePosition(unit->getPosition());
}


std::vector<osg::Vec3> Targets::getCityPositions() {
	return mCityPositions;
}

osg::Vec3 Targets::getNearestCityPosition(osg::Vec3 start) {
	std::vector<osg::Vec3>::iterator it;
	float nearest_dist = std::numeric_limits<float>::max();
	osg::Vec3 nearest;
	for (it = mCityPositions.begin(); it != mCityPositions.end(); it++) {
		float dist = ((*it)-start).length();
		if (dist < nearest_dist) {
			nearest_dist = dist;
			nearest = *it;
		}
	}
	return nearest;
}

osg::Vec3 Targets::getNearestCityPositionFrom(Unit* unit) {
	//TODO calc visibility from enemies
	return getNearestCityPosition(unit->getPosition());
}

std::vector<osg::Vec3> Targets::getGoalPositions() {
	return mGoalPositions;
}

osg::Vec3 Targets::getNearestGoalPosition(osg::Vec3 start) {
	std::vector<osg::Vec3>::iterator it;
	float nearest_dist = std::numeric_limits<float>::max();
	osg::Vec3 nearest;
	for (it = mGoalPositions.begin(); it != mGoalPositions.end(); it++) {
		float dist = ((*it)-start).length();
		if (dist < nearest_dist) {
			nearest_dist = dist;
			nearest = *it;
		}
	}
	return nearest;
}

osg::Vec3 Targets::getNearestGoalPositionFrom(Unit* unit) {
	//TODO calc visibility from enemies
	return getNearestGoalPosition(unit->getPosition());
}

/*std::vector<osg::Vec3> Targets::getVantagePoints() {

}

osg::Vec3 Targets::getNearestVantagePoint(osg::Vec3 start) {

}*/


std::vector<Unit*> Targets::findGroup(Unit* start, std::vector<Unit*>& units, float range) {
	//find any other units of the same side that are within range
	std::vector<Unit*> group;
	std::vector<Unit*> children;
 	group.push_back(start);
 	//compare the squared range
 	float d2 = range*range;
	for (std::vector<Unit*>::iterator it = units.begin(); it != units.end();) {
		float d;
		if (range <= 0) {
			d = pow(sqrt(start->getObservedPositionVariance()) + sqrt((*it)->getObservedPositionVariance()),2);
		} else {
			d = d2;
		}
		if ((*it)->getSide() == start->getSide() && ((*it)->getPosition()-start->getPosition()).length2() < d) {
			//save children
			children.push_back(*it);
			it = units.erase(it);
		} else {
			it++;
		}
	}
	for (unsigned int i=0; i < children.size(); i++) {
		//recurse down for each child found
		std::vector<Unit*> subgroup = findGroup(children[i], units, range);
		//merge subgroup into group
		std::copy(subgroup.begin(),subgroup.end(),std::back_inserter(group));
	}
	return group;
}
