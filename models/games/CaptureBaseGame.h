/*
 * EvasionGame.h
 *
 *  Created on: 26 Jan 2010
 *      Author: sbutler
 */

#ifndef CAPTUREBASEGAME_H_
#define CAPTUREBASEGAME_H_

#include <vector>
#include <map>
#include <osg/Vec3>

#include "Game.h"

class Unit;
class UnitGroup;
class Hypothesis;
class InverseModel;

class CaptureBaseGame : public Game {
public:
	CaptureBaseGame();
	virtual ~CaptureBaseGame();

	void initIMs();
	void initHypotheses();

	void updateGame();

private:
	void selectTarget(Unit* predator, std::vector<Unit*>& prey);

	std::vector<UnitGroup*> mAttackingGroups;
	std::vector<UnitGroup*> mDefendingGroups;

	std::map<UnitGroup*, InverseModel*> mDefenderModels;
	std::map<UnitGroup*, InverseModel*> mAttackerModels;
	std::map<UnitGroup*, Hypothesis*> mDefenderHypotheses;

	std::vector<osg::Vec3> mOffsets;
};

#endif /* CAPTUREBASEGAME_H_ */
