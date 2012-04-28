#include "InverseModelPath.h"
#include "../units/PathInterpolation.h"

#include "../HTAppBase.h"
#include "../models/inc/Targets.h"

InverseModelPath::InverseModelPath()
{
}

InverseModelPath::~InverseModelPath()
{
}

void InverseModelPath::createPath(Unit *unit, osg::Vec3 goal, bool set, bool marker, std::vector<UnitGroup*> avoid) {
	//add new point(s)
	std::vector< void* > path;
	float totalCost;
	osg::Vec3 pos;

	//do search
	if (set) {
		pos = unit->getPosition();
	} else {
		pos = unit->getFinalGoalPosition();
	}
	int result = theApp->getTerrainMap()->solve(pos, goal, &path, &totalCost, unit, avoid);

	if (result == micropather::MicroPather::SOLVED) {
		//first point
		/*osg::Vec3 pos = theApp->getTerrainMap()->stateToVec3(path[0]);
		if (set) {
			unit->setGoalPosition(pos);
		}*/
		PathInterpolation unitPath;
		//rest points
		for (unsigned int i=1; i<path.size(); i++) {
			osg::Vec3 pos = theApp->getTerrainMap()->stateToVec3(path[i]);
			//unit->addGoalPosition(pos);
			unitPath.addWaypoint(pos);
		}
		unit->setGoalPath(unitPath, set);
	} else {
		std::cout << "WARN: No path found" << std::endl;
	}
}
