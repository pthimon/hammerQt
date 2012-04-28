#ifndef TERRAINFEATURE_H_
#define TERRAINFEATURE_H_

#include <osg/Math>

enum feature_type {
  FEATURE_LOWHILL,
  FEATURE_HIGHHILL,
  FEATURE_FOREST,
  FEATURE_HOUSE
};

struct TerrainFeature
{
  float x, y;
  feature_type type;  
};

#endif /*TERRAINFEATURE_H_*/
