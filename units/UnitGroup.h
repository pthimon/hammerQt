/*
 * UnitGroup.h
 *
 *  Created on: 29-Apr-2009
 *      Author: sbutler
 */

#ifndef UNITGROUP_H_
#define UNITGROUP_H_

#include <osg/Vec3>
#include <osg/BoundingBox>
#include <osg/Referenced>
#include <vector>

#include "../common.h"

class Unit;

class UnitGroup : public osg::Referenced {
public:
	enum UnitGroupType { GROUP, OBSERVED_GROUP };

	UnitGroup(UnitSide side, UnitGroupType type = GROUP, float range = 0);
	UnitGroup(Unit* unit);
	virtual ~UnitGroup();

	void addUnits(std::vector<Unit*> units);
	void addUnit(Unit* unit);
	std::vector<Unit*> getUnits();
	std::vector<int> getUnitIDs();
	bool isInGroup(Unit* u);
	bool isValidGroup();

	void update();
	void updateBB(Unit* unit);

	osg::Vec3 getCentre();
	osg::Vec3 getCentreCurrent();
	osg::Vec3 getLOSCentre();

	osg::Vec3 getClosestUnitPosition(osg::Vec3 pos);

	float getRadius();
	float getRadiusX();
	float getRadiusY();

	osg::BoundingBox getBoundingBox();

	UnitSide getSide();

	void setGoalPosition(osg::Vec3 pos);
	void addGoalPosition(osg::Vec3 pos);

	float getMaxSpeed();

	float getRange();

private:
	void removeDeadUnits();

	UnitSide mSide;
	UnitGroupType mType;
	std::vector<Unit*> mUnits;
	osg::BoundingBox mBB;
	float mRange;
};

#endif /* UNITGROUP_H_ */
