#include "TerrainMap.h"

#include "../../units/Unit.h"
#include "../../units/UnitFiring.h"
#include "../../units/UnitGroup.h"
#include "Targets.h"

TerrainMap::TerrainMap(Terrain *terrain): mTerrain(terrain)
{
	mWidth = mTerrain->GetHeightField()->GetNumColumns();
	mHeight = mTerrain->GetHeightField()->GetNumRows();
	mHorizontalSize = mTerrain->GetHorizontalSize();
	mNodeWidth = getNodeWidth();

	mMaxGradient = 0.3;
	mMaxSpeed = 1;
}

TerrainMap::~TerrainMap()
{
}

//A* functions
float TerrainMap::LeastCostEstimate( void* stateStart, void* stateEnd ) {
	osg::Vec2 startPos = stateToVec(stateStart);
	osg::Vec2 endPos = stateToVec(stateEnd);

	osg::Vec3 startPos3D(startPos[0], startPos[1], getHeight(startPos)/mNodeWidth);
	osg::Vec3 endPos3D(endPos[0], endPos[1], getHeight(endPos)/mNodeWidth);

	return (endPos3D - startPos3D).length() / mMaxSpeed;
}

void TerrainMap::AdjacentCost( void* state, std::vector< micropather::StateCost > *neighbors ) {

	//TODO some tiles need to be marked as unreachable to avoid the code searching every possible tile for a route

	osg::Vec2 pos = stateToVec(state);
	//double currentVisibleCost = getIsVisibleCost(state);

	//the 8 adjacent positions
	const int dx[8] = { 1, 1, 0, -1, -1, -1,  0,  1 };
	const int dy[8] = { 0, 1, 1,  1,  0, -1, -1, -1 };
	const float dist[8] = { 1.0f, 1.414213562373095049f, 1.0f, 1.414213562373095049f, 1.0f, 1.414213562373095049f, 1.0f, 1.414213562373095049f };
	const float dist2[8] = { 1.0f, 2.0f, 1.0f, 2.0f, 1.0f, 2.0f, 1.0f, 2.0f };

	float nodeHeight = getHeight(pos);
	osg::Vec2 world = mapToWorldCoords(pos);
	float nodeSpeed = mUnit->getSpeed(world[0],world[1]);

	for( int i=0; i<8; ++i ) {
		int nx = pos.x() + dx[i];
		int ny = pos.y() + dy[i];

		if ((nx >= 0) && (ny >= 0) && (nx < mWidth) && (ny < mHeight)) {

			float newHeight = getHeight(osg::Vec2(nx,ny));
			float heightDiff = (newHeight - nodeHeight)/ mNodeWidth;
			float gradient = heightDiff / dist[i];
			osg::Vec2 nworld = mapToWorldCoords(osg::Vec2(nx,ny));

			if (fabs(gradient) < mMaxGradient) {
				float newSpeed = mUnit->getSpeed(nworld[0],nworld[1]);
				//calc cost, time = dist / speed
				float dist3D = sqrt(dist2[i] + heightDiff*heightDiff);
				//average speed to go from here to there
				float avspeed = (nodeSpeed + newSpeed) / 2;
				float cost = dist3D / avspeed;

				void* nextState = vecToState( nx, ny );

				cost += getIsVisibleCost(nextState);

				micropather::StateCost nodeCost = { nextState, cost };
				neighbors->push_back( nodeCost );
				continue;
			}
		}
		//otherwise no go
		micropather::StateCost nodeCost = { vecToState( nx, ny ), FLT_MAX };
		neighbors->push_back( nodeCost );
	}
}

void TerrainMap::PrintStateInfo( void* state ) {
	osg::Vec2 node = stateToVec(state);
	std::cout << node.x() << "," << node.y() << std::endl;
}

//Utility functions

osg::Vec2 TerrainMap::worldToMapCoords(osg::Vec2 vec) {
	int col = floor((vec.x()/mHorizontalSize + 0.5)*mWidth);
	int row = floor((vec.y()/mHorizontalSize + 0.5)*mHeight);
	return osg::Vec2(col, row);
}

osg::Vec2 TerrainMap::mapToWorldCoords(osg::Vec2 vec) {
	//put us in the middle of the map square
	float fx = (((vec.x() / mWidth) - 0.5) * mHorizontalSize) + (mHorizontalSize / (2*mWidth));
	float fy = (((vec.y() / mHeight) - 0.5) * mHorizontalSize) + (mHorizontalSize / (2*mWidth));
	return osg::Vec2(fx, fy);
}

//in height map coordinates
//returns height map coordinates
osg::Vec2 TerrainMap::stateToVec(void* state) {
	int node = (long int)state;
	int y = node / mWidth;
	int x = node - y*mWidth;
	return osg::Vec2(x,y);
}

//in height map coordinates
//returns world coordinates
osg::Vec3 TerrainMap::stateToVec3(void* state) {
	osg::Vec2 mapPos = stateToVec(state);
	osg::Vec2 worldPos = mapToWorldCoords(mapPos);
	float z = mTerrain->GetHeight(worldPos.x(),worldPos.y());
	return osg::Vec3(worldPos.x(),worldPos.y(),z);
}

//in world coordinates
//returns height map coordinates
void *TerrainMap::vecToState(osg::Vec2 vec) {
	osg::Vec2 mapPos = worldToMapCoords(vec);
	return vecToState(mapPos.x(), mapPos.y());
}

//in world coordinates
//returns height map coordinates
void *TerrainMap::vecToState(osg::Vec3 vec) {
	return vecToState(osg::Vec2(vec.x(),vec.y()));
}

//in height map coordinates
//returns height map coordinates
void *TerrainMap::vecToState(int x, int y) {
	return (void *)((long int)y*mWidth + (long int)x);
}

//in height map coordinates
//returns world coord
double TerrainMap::getHeight(osg::Vec2 pos) {
	osg::Vec2 worldPos = mapToWorldCoords(pos);
	return mTerrain->GetHeight(worldPos.x(),worldPos.y());
}

double TerrainMap::getIsVisibleCost(void* state) {
	osg::Vec2 pos = stateToVec(state);
	//otherwise check line of sight
	float maxDist = std::numeric_limits<float>::max();
	float maxRange = 1;
	osg::Vec3 pos3d = stateToVec3(state);
	//if we're going into a forest then we're not visible
	unsigned long int layerMask = mTerrain->GetLayerBitsAt((int)pos[0], (int)pos[1]);
	if (layerMask & mForestMask) {
		return 0;
	}
	pos3d.z() += 2;
	for (unsigned int i=0; i < mEnemyPositions.size(); i++) {
		osg::Vec3 enemyPos = mEnemyPositions[i];
		float range = mEnemyRanges[i];
		float dist = (enemyPos - pos3d).length2();
		//only do line of sight if we're within range and closer than an already found LOS
		if (dist < range && dist < maxDist) {
			if (mTerrain->IsClearLineOfSight(pos3d, enemyPos)) {
				maxDist = dist;
				maxRange = range;
			}
		}
	}
	if (maxDist < std::numeric_limits<float>::max()) {
		return 10*(maxRange - maxDist)/maxRange;
	} else {
		return 0;
	}
}

int TerrainMap::solve(osg::Vec3 startPos, osg::Vec3 endPos, std::vector<osg::Vec2>& path, float& totalCost, Unit* unit, std::vector<UnitGroup*> avoid) {
	std::vector<void*> p;
	int result = solve(startPos, endPos, &p, &totalCost, unit, avoid);

	if (result == micropather::MicroPather::SOLVED) {
		for(std::vector<void*>::iterator it = p.begin(); it != p.end(); it++) {
			path.push_back(stateToVec(*it));
		}
	}
	return result;
}

//the A* solver
int TerrainMap::solve(osg::Vec3 startPos, osg::Vec3 endPos, std::vector<void*> *path, float *totalCost, Unit* unit, std::vector<UnitGroup*> avoid) {
	mUnit = unit;
	mMaxSpeed = unit->getMaxSpeed();
	mForestMask = 1l << theApp->getTerrain()->GetLayerIndex("forest");

	//std::vector<UnitGroup*> avoid = theApp->getTargets()->getUnitGroups(BLUE);
	for (unsigned int i=0; i < avoid.size(); i++) {
		osg::Vec3 enemyPos = avoid[i]->getLOSCentre();
		enemyPos.z() += 2;
		mEnemyPositions.push_back(enemyPos);
		mEnemyRanges.push_back(avoid[i]->getRange());
	}

	/*if (getIsVisibleCost(vecToState(endPos)) > 0) {
		//end point is visible, so give up doing avoiding routing
		mEnemyPositions.clear();
	}*/

	micropather::MicroPather pather( this );

	int result = pather.Solve( vecToState(startPos), vecToState(endPos), path, totalCost );
	
	//TODO optimize path
	//remove nodes inbetween those that lie on a straight line
	//REMOVED because it means the path is now made up of long sweeping curves due to the interpolation
	/*osg::Vec2 behindPos = stateToVec(path->at(0));
	osg::Vec2 midPos = stateToVec(path->at(1));
	osg::Vec2 currPos;
	for (std::vector<void*>::iterator pathIt = path->begin()+1; pathIt != path->end()-1; ) {
		currPos = stateToVec(*(pathIt+1));

		//std::cout << currPos.x() << "," << midPos.x() << "," << behindPos.x() << std::endl;
		//std::cout << currPos.y() << "," << midPos.y() << "," << behindPos.y() << std::endl;

		if ((currPos.x() == midPos.x() && midPos.x() == behindPos.x()) ||
				(currPos.y() == midPos.y() && midPos.y() == behindPos.y()) ||
				(currPos.x()+1 == midPos.x() && midPos.x()+1 == behindPos.x() && currPos.y()+1 == midPos.y() && midPos.y()+1 == behindPos.y()) ||
				(currPos.x()+1 == midPos.x() && midPos.x()+1 == behindPos.x() && currPos.y()-1 == midPos.y() && midPos.y()-1 == behindPos.y()) ||
				(currPos.x()-1 == midPos.x() && midPos.x()-1 == behindPos.x() && currPos.y()+1 == midPos.y() && midPos.y()+1 == behindPos.y()) ||
				(currPos.x()-1 == midPos.x() && midPos.x()-1 == behindPos.x() && currPos.y()-1 == midPos.y() && midPos.y()-1 == behindPos.y())) {
			//std::cout << ">> erasing " << midPos.x() << "," << midPos.y() << std::endl;
			path->erase(pathIt);
		} else {
			pathIt++;
		}

		behindPos = midPos;
		midPos = currPos;
	}*/

	return result;
}
