#ifndef COMMON_H_
#define COMMON_H_

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wignored-qualifiers"

#include <cstddef>
#include <dtCore/refptr.h>
#include <dtCore/deltadrawable.h>
#include <osg/BoundingBox>
#include <ostream>
#include <osg/Vec3>

//This is for the CEGUI headers.
#ifdef None
#undef None
#endif
#include <CEGUI/CEGUI.h>

#define HT_MAX(A,B) ( (A) > (B) ) ? (A) : (B)
#define HT_MIN(A,B) ( (A) < (B) ) ? (A) : (B)

typedef CEGUI::Window* MarkerType;

enum UnitSide {
	ALL,
	NONE,
	ACTIVE,
	RED,
	BLUE
};

enum UnitType {
	TANK,
	SOLDIER
};

enum UnitInstance {
	LOCAL,
	REMOTE
};

enum UnitObserverType {
	GLOBAL,
	SEQUENTIAL,
	THREAT,
	FOLLOWING,
	ROUND_ROBIN
};

class HTAppBase;

extern dtCore::RefPtr<HTAppBase> theApp;

// global functions
bool GetBoundingBox(osg::BoundingBox & BB, dtCore::DeltaDrawable & drawable);

float random(float min,float max);
int random(int min,int max);

#endif /*COMMON_H_*/
