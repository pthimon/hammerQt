/*
 * InverseModelTrack.h
 *
 *  Created on: 26 Jan 2010
 *      Author: sbutler
 */

#ifndef INVERSEMODELTRACK_H_
#define INVERSEMODELTRACK_H_

#include "inc/InverseModel.h"

class InverseModelTrack: public InverseModel {
public:
	InverseModelTrack(Unit* predator, bool path = false);
	virtual ~InverseModelTrack();

	void update();

	void setTarget(Unit* u);
	void setPath(Unit *unit, osg::Vec3 goal);

	Unit* getTarget();

	void removeUnit(Unit* unit, bool clear);
	bool valid();

private:
	Unit* mPredator;
	Unit* mPrey;
	bool mDoPath;
};

#endif /* INVERSEMODELTRACK_H_ */
