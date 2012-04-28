#ifndef INVERSEMODELCONVOY_H_
#define INVERSEMODELCONVOY_H_

#include "inc/InverseModel.h"

class UnitGroup;

class InverseModelConvoy : public InverseModel
{
public:
	InverseModelConvoy(std::vector<Unit*>& units);
	virtual ~InverseModelConvoy();

	virtual void update();
	void removeUnit(Unit* unit, bool clear);
	void allocate();
protected:
	std::vector<Unit*> mUnits;
	UnitGroup* mGroup;
	osg::Vec3 mCityPosition;
};

#endif /*INVERSEMODELCONVOY_H_*/
