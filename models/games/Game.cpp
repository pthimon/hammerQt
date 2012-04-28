/*
 * DefaultGame.cpp
 *
 *  Created on: 15 Feb 2010
 *      Author: sbutler
 */

#include "Game.h"

#include "../../common.h"
#include "../../hypotheses/HypothesisManager.h"
#include "../../HTAppBase.h"

#include <dtCore/globals.h>

Game::Game() :
	mReplay(false),
	mManager(NULL),
	mReplayIndex(0),
	mInit(false)
{
}

Game::~Game() {
	// TODO Auto-generated destructor stub
}

void Game::setReplay(std::string replayFile) {
	//parse replay for this game
	std::string replayPath = "";
	if (!replayFile.empty()) {
		replayPath = dtCore::FindFileInPathList( "results/logs/" + replayFile );
		if (replayPath.empty()) {
			//assume an absolute filename
			replayPath = replayFile;
		}
	} else {
		return;
	}

	std::ifstream ifs(replayPath.c_str(),std::ifstream::in);
	if (!ifs.is_open()) {
		return;
	}

	mReplay = true;

	double timeStamp;
	//parse log file for GOAL commands
	while (ifs.good()) {
		std::string command;
		ifs >> timeStamp >> command;
		GameReplayCommand* c = parseReplayCommand(ifs, timeStamp, command);
		if (c) {
			mReplayCommands.push_back(c);
		} else {
			//discard
			ifs.ignore(1024,'\n');
		}
	}
}

bool Game::isReplay() {
	return mReplay;
}

/**
 * Call the overridden member functions to initialise the IMs and Hypotheses
 */
void Game::init(HypothesisManager* manager) {
	//save ref to hypothesis manager
	mManager = manager;

	initIMs();

	if (mManager) {
		initHypotheses();
		mManager->setInitialised();
	}

	mInit = true;
}

void Game::update() {
	if (isReplay() && !mReplayCommands.empty() && (mReplayIndex < mReplayCommands.size())) {
		//set new target if it is time
		double simTime = theApp->getSimTime();
		GameReplayCommand* c = mReplayCommands[mReplayIndex];
		while (c->getTime() <= simTime) {
			//apply command
			applyReplayCommand(c);
			delete c;
			if (++mReplayIndex == mReplayCommands.size()) {
				break;
			}
			c = mReplayCommands[mReplayIndex];
		}
	}

	updateGame();
}
