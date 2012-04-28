#ifndef TERRAINMAP_H_
#define TERRAINMAP_H_

#include "../../terrain/terrain.h"
#include "micropather.h"

class Unit;
class UnitGroup;

class TerrainMap : public micropather::Graph
{
public:
	TerrainMap(Terrain *terrain);
	virtual ~TerrainMap();

	//A* functions
	float LeastCostEstimate( void* stateStart, void* stateEnd );

	void AdjacentCost( void* state, std::vector< micropather::StateCost > *neighbors );

	void PrintStateInfo( void* state );

	//the A* solver
	int solve(osg::Vec3 startPos, osg::Vec3 endPos, std::vector<void*> *path, float *totalCost, Unit* unit, std::vector<UnitGroup*> avoid = std::vector<UnitGroup*>());
	//overloaded for convenience to get a Vec3 path
	int solve(osg::Vec3 startPos, osg::Vec3 endPos, std::vector<osg::Vec2>& path, float& totalCost, Unit* unit, std::vector<UnitGroup*> avoid = std::vector<UnitGroup*>());

	osg::Vec3 stateToVec3(void* state);

	float getNodeWidth() { return mHorizontalSize / mWidth; }

	osg::Vec2 worldToMapCoords(osg::Vec2 vec);
	osg::Vec2 mapToWorldCoords(osg::Vec2 vec);
private:
	//Utility functions

	osg::Vec2 stateToVec(void* state);

	void *vecToState(osg::Vec2 vec);
	void *vecToState(osg::Vec3 vec);
	void *vecToState(int x, int y);

	double getHeight(osg::Vec2 vec);
	double getIsVisibleCost(void* state);

	Terrain *mTerrain;
	int mWidth;
	int mHeight;
	float mHorizontalSize;
	float mNodeWidth;
	float mVerticalScale;

	float mMaxGradient;

	dtCore::RefPtr<Unit> mUnit;
	float mMaxSpeed;

	std::vector<osg::Vec3> mEnemyPositions;
	std::vector<float> mEnemyRanges;
	unsigned long int mForestMask;
};

#endif /*TERRAINMAP_H_*/
