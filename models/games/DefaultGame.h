/*
 * DefaultGame.h
 *
 *  Created on: 15 Feb 2010
 *      Author: sbutler
 */

#ifndef DEFAULTGAME_H_
#define DEFAULTGAME_H_

#include "Game.h"

class DefaultGame: public Game {
public:
	DefaultGame();
	virtual ~DefaultGame();

	void initHypotheses();

	void updateGame();
};

#endif /* DEFAULTGAME_H_ */
