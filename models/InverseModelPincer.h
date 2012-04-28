/*
 * InverseModelPincer.h
 *
 *  Created on: 8 Dec 2009
 *      Author: sbutler
 */

#ifndef INVERSEMODELPINCER_H_
#define INVERSEMODELPINCER_H_

#include "inc/InverseModel.h"

class Unit;
class UnitGroup;

class InverseModelPincer : public InverseModel {
public:
	InverseModelPincer(std::vector<Unit*> units);
	virtual ~InverseModelPincer();

	virtual void update();

protected:
	bool divideGroup();
	float findCircle(std::vector<osg::Vec3> p, osg::Vec2 &c);

	osg::Vec3 mTarget;
	std::vector<Unit*> mUnits;
	dtCore::RefPtr<UnitGroup> mGroup1;
	dtCore::RefPtr<UnitGroup> mGroup2;
};

#endif /* INVERSEMODELPINCER_H_ */
