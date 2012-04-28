#include "common.h"

#pragma GCC diagnostic ignored "-Wextra"

#include <dtUtil/boundingshapeutils.h>
#include <dtUtil/log.h>
#include <climits>
#include <cstdlib>

#include <cstdlib>

#pragma GCC diagnostic warning "-Wextra"

bool GetBoundingBox(osg::BoundingBox & BB, dtCore::DeltaDrawable & drawable)
{
    osg::Node* node = drawable.GetOSGNode();
    if (node != 0)
    {
        dtUtil::BoundingBoxVisitor bbv;
		node->accept(bbv);

		// no copy constructor for osg::BB...
		BB.set(bbv.mBoundingBox._min, bbv.mBoundingBox._max);
		return true;
    }

    LOG_WARNING("No valid osg node of drawable when asking for bounding box.");
    return false;
}

float random(float min,float max)
  {
    return min + (max-min)*(float)rand()/(float)RAND_MAX;
  }

int random(int min,int max)
  {
    return min + (int)((float)(max-min)*(float)rand()/(float)RAND_MAX);
  }
