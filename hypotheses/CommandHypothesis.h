/*
 * TargetDirectionHypothesis.h
 *
 *  Created on: 18 Jan 2010
 *      Author: sbutler
 */

#ifndef COMMANDHYPOTHESIS_H_
#define COMMANDHYPOTHESIS_H_

#include "Hypothesis.h"

class CommandHypothesis: public Hypothesis {
public:
	CommandHypothesis(std::vector<UnitGroup*> blueUnits, UnitGroup* myUnits, Command::CommandType command, std::string name);
	virtual ~CommandHypothesis();
};

#endif /* COMMANDHYPOTHESIS_H_ */
