#ifndef INVERSEMODELATTACK_H_
#define INVERSEMODELATTACK_H_

#include "inc/InverseModel.h"

class InverseModelAttack : public InverseModel
{
public:
	InverseModelAttack(std::vector<Unit*>& units);
	virtual ~InverseModelAttack();

	virtual void update();
	void removeUnit(Unit* unit, bool clear);
	void allocate();

protected:
	std::vector<Unit*> mUnits;
	std::vector<std::pair<Unit*,Unit*> > mAllocation;
};

#endif /*INVERSEMODELATTACK_H_*/
