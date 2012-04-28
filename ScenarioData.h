#ifndef SCENARIODATA_H_
#define SCENARIODATA_H_

#include <string>
#include <vector>
#include <osg/Vec2>

#include "units/Unit.h"

class ScenarioUnitData 
{
public:
	ScenarioUnitData();
	ScenarioUnitData(UnitType unitType, int unitInstance, int unitNumber);
	virtual ~ScenarioUnitData();
	
	UnitType type;
	int instance;
	std::vector<osg::Vec2> positions;
	int number;
	
	void setType(std::string& typeStr);
	void addStartOrEndPosition(osg::Vec2 pos);
	void addPosition(osg::Vec2 pos);
};

class ScenarioSideData 
{
public:
	ScenarioSideData();
	ScenarioSideData(UnitSide unitSide);
	virtual ~ScenarioSideData();
	
	UnitSide side;
	int total;
	std::vector<ScenarioUnitData*> units;
	
	void setSide(std::string& name);
};

class ScenarioData
{
public:
	ScenarioData();
	ScenarioData(std::string map, int controller);
	virtual ~ScenarioData();
	
	std::string mapName;
	// stores the time limits in seconds
	unsigned int goalTime, timeLimit;
	int controllerId;
	bool forwardModel;
	std::vector<ScenarioSideData*> sides;
	int numInstances;
	std::string gameType;
	float simTime;
	float speedup;
	bool autoFire;
};

#endif /*SCENARIODATA_H_*/
