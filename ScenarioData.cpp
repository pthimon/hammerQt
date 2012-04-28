#include "ScenarioData.h"

//ScenarioUnitData

ScenarioUnitData::ScenarioUnitData() : type(TANK), instance(0), number(0)
{
}

ScenarioUnitData::ScenarioUnitData(UnitType unitType, int unitInstance, int unitNumber) :
	type(unitType), instance(unitInstance), number(unitNumber)
{
}

ScenarioUnitData::~ScenarioUnitData()
{
}

void ScenarioUnitData::setType(std::string& typeStr)
{
	if (typeStr == "tank") {
		type = TANK;
	} else if (typeStr == "soldier") {
		type = SOLDIER;
	}
}

void ScenarioUnitData::addStartOrEndPosition(osg::Vec2 pos)
{
	positions.push_back(pos);
}

void ScenarioUnitData::addPosition(osg::Vec2 pos)
{
	positions.push_back(pos);
	number++;
}

//ScenarioSideData

ScenarioSideData::ScenarioSideData() : side(BLUE), total(0)
{
}

ScenarioSideData::ScenarioSideData(UnitSide sideParam) : side(sideParam)
{
}

ScenarioSideData::~ScenarioSideData()
{
}

void ScenarioSideData::setSide(std::string& name) 
{
	if (name == "blue") {
		side = BLUE;
	} else if (name == "red") {
		side = RED;
	}
}

//ScenarioData

ScenarioData::ScenarioData() : mapName("world1"), controllerId(std::numeric_limits<int>::min()), forwardModel(false), numInstances(0), simTime(5), speedup(4), autoFire(true)
{
}

ScenarioData::ScenarioData(std::string map, int controller) : mapName(map), controllerId(controller)
{
}

ScenarioData::~ScenarioData()
{
}
