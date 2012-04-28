#ifndef INVERSEMODELRETREAT_H_
#define INVERSEMODELRETREAT_H_

#include "inc/InverseModel.h"

class InverseModelRetreat : public InverseModel
{
public:
	InverseModelRetreat(std::vector<Unit*>& units);
	virtual ~InverseModelRetreat();

	virtual void update();
	void removeUnit(Unit* unit, bool clear);
	void allocate();
protected:
	std::vector<Unit*> mUnits;
	std::vector<std::pair<Unit*,Unit*> > mAllocation;
};

#endif /*INVERSEMODELRETREAT_H_*/
