/*
 * Targets.h
 *
 *  Created on: 03-Apr-2009
 *      Author: sbutler
 */

#ifndef TARGETS_H_
#define TARGETS_H_

#include <vector>
#include <set>
#include <limits>
#include <osg/Vec3>

#include "../../common.h"

class Terrain;
class Unit;
class UnitGroup;

class Targets {
public:
	Targets(Terrain *terrain, std::set< dtCore::RefPtr<Unit> > *units, std::vector<dtCore::RefPtr<UnitGroup> > *groups);
	virtual ~Targets();

	/**
	 * Called by HTAppBase::PreFrame to group units
	 */
	void update();
	void updateObservations(UnitSide side);

	std::vector<dtCore::RefPtr<UnitGroup> > groupUnits(float range, UnitSide side = ALL);
	std::vector<dtCore::RefPtr<UnitGroup> > groupObservedUnits(UnitSide side);
	UnitGroup* groupObservedUnits(UnitSide side, UnitGroup* start, float range);

	/**
	 * Returns all of the units that are alive. Valid within the preframe function
	 *
	 * @param side		returns all the units that are assigned to the side
	 * @param start		centre of the radius
	 * @param radius	returns all the units within the radius of the start position
	 */
	std::vector<Unit*> getUnits(UnitSide side = ALL, osg::Vec3 start = osg::Vec3(), float radius = 0);

	/**
	 * Same as getUnits except a vector of clones of the active units is returned
	 *
	 * The cloned units are allocated on the heap and should be deleted when no longer needed
	 */
	std::vector<Unit*> getUnitSnapshots(UnitSide side, osg::Vec3 start, float radius);

	std::vector<Unit*> getObservedUnits(UnitSide side = ALL);

	std::vector<int> getUnitIDs(UnitSide side = ALL, osg::Vec3 start = osg::Vec3(), float radius = 0);

	std::vector<UnitGroup*> getUnitGroups(UnitSide side = ALL);

	std::vector<UnitGroup*> getObservedUnitGroups(UnitSide side = ALL);

	/**
	 * Returns the defensive positions in the terrain
	 */
	std::vector<osg::Vec3> getDefensivePositions();

	/**
	 * Finds the nearest defensive terrain feature to a point
	 */
	osg::Vec3 getNearestDefensivePosition(osg::Vec3 start);

	/**
	 * Calculates the nearest position to a unit that is hidden from view of
	 * the enemies
	 */
	osg::Vec3 getNearestDefensivePositionFrom(Unit* unit);

	/**
	 * Returns the defensive positions in the terrain
	 */
	std::vector<osg::Vec3> getCityPositions();

	/**
	 * Finds the nearest defensive terrain feature to a point
	 */
	osg::Vec3 getNearestCityPosition(osg::Vec3 start);

	/**
	 * Calculates the nearest position to a unit that is hidden from view of
	 * the enemies
	 */
	osg::Vec3 getNearestCityPositionFrom(Unit* unit);

	/**
	 * Returns the defensive positions in the terrain
	 */
	std::vector<osg::Vec3> getGoalPositions();

	/**
	 * Finds the nearest defensive terrain feature to a point
	 */
	osg::Vec3 getNearestGoalPosition(osg::Vec3 start);

	/**
	 * Calculates the nearest position to a unit that is hidden from view of
	 * the enemies
	 */
	osg::Vec3 getNearestGoalPositionFrom(Unit* unit);

	/**
	 * From a starting unit, find other units within a certain range
	 */
	std::vector<Unit*> findGroup(Unit* start, std::vector<Unit*>& units, float range);

private:

	Terrain *mTerrain;
	std::set< dtCore::RefPtr<Unit> > *mUnits;
	std::vector<dtCore::RefPtr<UnitGroup> > *mGroups;
	std::vector<dtCore::RefPtr<UnitGroup> > mObservedGroups;
	std::vector<osg::Vec3> mDefensivePositions;
	std::vector<osg::Vec3> mCityPositions;
	std::vector<osg::Vec3> mGoalPositions;
};

#endif /* TARGETS_H_ */
