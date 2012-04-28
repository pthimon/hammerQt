#ifndef INVERSEMODELDEFEND_H_
#define INVERSEMODELDEFEND_H_

#include "inc/InverseModel.h"

class UnitGroup;

class InverseModelDefend : public InverseModel
{
public:
	InverseModelDefend(std::vector<Unit*>& units);
	virtual ~InverseModelDefend();

	virtual void update();
	void removeUnit(Unit* unit, bool clear);
	void allocate();
protected:
	std::vector<Unit*> mUnits;
	UnitGroup* mGroup;
	std::vector<int> mArrived;
	osg::Vec3 mDefensivePosition;
};

#endif /*INVERSEMODELDEFEND_H_*/
