#ifndef INVERSEMODEL_H_
#define INVERSEMODEL_H_

#include <algorithm>

#include "../../common.h"
#include "../../units/Unit.h"

class InverseModel : public osg::Referenced
{
public:
	InverseModel();
	virtual ~InverseModel();

	virtual bool valid() { return true; };
	virtual void update() {};
	virtual void addUnit(Unit *unit);
	virtual void removeUnit(Unit *unit, bool clear);

	virtual Unit* getNearestEnemyUnitTo(Unit *unit);

protected:
	std::vector<Unit*> mAllUnits;
};

#endif /*INVERSEMODEL_H_*/
