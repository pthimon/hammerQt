/*
 * EvasionGame.h
 *
 *  Created on: 26 Jan 2010
 *      Author: sbutler
 */

#ifndef MAZEGAME_H_
#define MAZEGAME_H_

#include <vector>
#include <map>
#include <osg/Vec3>

#include "Game.h"

class Unit;
class InverseModelTrack;
class Hypothesis;

class MazeGame : public Game {
public:
	MazeGame();
	virtual ~MazeGame();

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
};

#endif /* MAZEGAME_H_ */
