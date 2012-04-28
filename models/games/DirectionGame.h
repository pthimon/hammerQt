/*
 * EvasionGame.h
 *
 *  Created on: 26 Jan 2010
 *      Author: sbutler
 */

#ifndef DIRECTIONGAME_H_
#define DIRECTIONGAME_H_

#include <vector>
#include <map>
#include <osg/Vec3>

#include "Game.h"

class Unit;
class InverseModelTrack;
class Hypothesis;

class DirectionGame : public Game {
public:
	DirectionGame();
	virtual ~DirectionGame();

	void initIMs();
	void initHypotheses();

	GameReplayCommand* parseReplayCommand(std::ifstream& ifs, double timeStamp, std::string command);
	void applyReplayCommand(GameReplayCommand* c);

	void updateGame();

	void setNumLevels(int levels) {
		mNumLevels = levels;
	}
	void setNumSegments(int segments) {
		mNumSegments = segments;
	}

private:
	void selectTarget(Unit* predator, std::vector<Unit*>& prey);

	std::map<Unit*, InverseModelTrack*> mPredatorModels;
	std::map<Unit*, Hypothesis*> mPredatorHypotheses;
	std::map<Unit*, std::vector<int> > mPredatorTargets;

	int mNumLevels;
	int mNumSegments;
};

#endif /* DIRECTIONGAME_H_ */
