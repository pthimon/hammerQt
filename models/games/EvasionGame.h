/*
 * EvasionGame.h
 *
 *  Created on: 26 Jan 2010
 *      Author: sbutler
 */

#ifndef EVASIONGAME_H_
#define EVASIONGAME_H_

#include <vector>
#include <map>
#include <osg/Vec3>

#include "Game.h"

class Unit;
class InverseModelTrack;
class Hypothesis;

class EvasionGame : public Game {
public:
	EvasionGame();
	virtual ~EvasionGame();

	void initIMs();
	void initHypotheses();

	GameReplayCommand* parseReplayCommand(std::ifstream& ifs, double timeStamp, std::string command);
	void applyReplayCommand(GameReplayCommand* c);

	void updateGame();

private:
	void selectTarget(Unit* predator, std::vector<Unit*>& prey);

	std::map<Unit*, InverseModelTrack*> mPredatorModels;
	std::map<Unit*, Hypothesis*> mPredatorHypotheses;
	std::map<Unit*, std::vector<int> > mPredatorTargets;

	std::vector<osg::Vec3> mOffsets;
};

#endif /* EVASIONGAME_H_ */
