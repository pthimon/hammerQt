#ifndef INVERSEMODELPATH_H_
#define INVERSEMODELPATH_H_

#include "inc/InverseModel.h"

class UnitGroup;

class InverseModelPath : public InverseModel
{
public:
	InverseModelPath();
	virtual ~InverseModelPath();

	static void createPath(Unit *unit, osg::Vec3 goal, bool set=true, bool marker=false, std::vector<UnitGroup*> avoid = std::vector<UnitGroup*>());
};

#endif /*INVERSEMODELPATH_H_*/
