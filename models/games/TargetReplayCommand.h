/*
 * EvasionGame.h
 *
 *  Created on: 26 Jan 2010
 *      Author: sbutler
 */

#ifndef TARGETREPLAYCOMMAND_H_
#define TARGETREPLAYCOMMAND_H_

#include "Game.h"

class TargetReplayCommand: public GameReplayCommand {
public:
	TargetReplayCommand(double time, Unit* predator, Unit* prey): GameReplayCommand(time), mPredator(predator), mPrey(prey) {};
	virtual ~TargetReplayCommand() {};
	Unit* getPredator() { return mPredator; };
	Unit* getPrey() { return mPrey; };
protected:
	Unit* mPredator;
	Unit* mPrey;
};

#endif
