/*
 * Game.h
 *
 *  Created on: 26 Jan 2010
 *      Author: sbutler
 */

#ifndef GAME_H_
#define GAME_H_

#include <fstream>
#include <string>
#include <vector>

class HypothesisManager;

class GameReplayCommand {
public:
	GameReplayCommand(double time): mTime(time) {};
	virtual ~GameReplayCommand() {};
	double getTime() { return mTime; };
protected:
	double mTime;
};

class Game {
public:
	Game();
	virtual ~Game();

	void setReplay(std::string replay);
	bool isReplay();
	virtual GameReplayCommand* parseReplayCommand(std::ifstream&, double, std::string) { return NULL; };
	virtual void applyReplayCommand(GameReplayCommand*) {};

	virtual void init(HypothesisManager* manager = NULL);

	virtual void initIMs() {};
	virtual void initHypotheses() {};

	void update();
	virtual void updateGame() {};

	virtual bool isInitialised() { return mInit; }

protected:
	bool mReplay;
	HypothesisManager* mManager;
	std::vector<GameReplayCommand*> mReplayCommands;
	unsigned int mReplayIndex;
	bool mInit;
};

#endif /* GAME_H_ */
