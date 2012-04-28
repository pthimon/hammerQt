#include "PathInterpolation.h"
#include <iostream>
#include <dtUtil/log.h>

IMPLEMENT_MANAGEMENT_LAYER(PathInterpolation)

PathInterpolation::PathInterpolation(double range, double stepSize): 
	mWaypointIndex(0), 
	mTi(0), 
	mStepSize(stepSize),
	m_LastStart(0)
	{
	mRange = range * range;
}

PathInterpolation::~PathInterpolation()
{
}

//clear all waypoints, set new one
void PathInterpolation::clearWaypoints() {
	mWaypointsX.clear();
	mWaypointsY.clear();
	mWaypointsZ.clear();
	mT.clear();
	//mWaypointIndex = -1;
	mWaypointIndex = 0;
	mTi = 0;
}

void PathInterpolation::addWaypoint(osg::Vec3 p) {
	mPrevAddedWaypoint = mLastAddedWaypoint;
	mLastAddedWaypoint = p;
	
	mWaypointsX.push_back(p.x());
	mWaypointsY.push_back(p.y());
	mWaypointsZ.push_back(p.z());
	
        if (mT.size() < 1) {
                //first waypoint
                mT.push_back(0);
        } else {
                //add on path length
                mT.push_back(mT[mT.size()-1] + (mLastAddedWaypoint - mPrevAddedWaypoint).length());
        }
	
        generateSpline((mWaypointIndex < 0) ? 0 : mWaypointIndex);
}

osg::Vec3 PathInterpolation::getWaypoint(int index) {
	return osg::Vec3(mWaypointsX.at(index), mWaypointsY.at(index), mWaypointsZ.at(index));
}

void PathInterpolation::generateSpline(int start) {
	n = mT.size() - start;
	m_LastStart = start;
	double* xpp_spline;
	double* ypp_spline;
	double* zpp_spline;
	if ( n != 1)
	  {
	    xpp_spline = spline_cubic_set ( n, &mT[start], &mWaypointsX[start], 0, 0, 0, 0 );
	    ypp_spline = spline_cubic_set ( n, &mT[start], &mWaypointsY[start], 0, 0, 0, 0 );
	    zpp_spline = spline_cubic_set ( n, &mT[start], &mWaypointsZ[start], 0, 0, 0, 0 );
	  }
	else if (n == 1)
	  {
	    xpp_spline = &mWaypointsX[start];
            ypp_spline = &mWaypointsY[start];
            zpp_spline = &mWaypointsZ[start];
	  }
	else
	  {
	      LOG_WARNING("Spline generation was called with for a 0 length.");
	      return;
	  }
	      	
	// copy these into std::vectors
	xpp.assign(&xpp_spline[0], &xpp_spline[n-1]);
        ypp.assign(&ypp_spline[0], &ypp_spline[n-1]);
        zpp.assign(&zpp_spline[0], &zpp_spline[n-1]);
}

osg::Vec3 PathInterpolation::getInterpolatedPoint(double ti) {
	double xval = spline_cubic_val ( n, &mT[m_LastStart], ti, &mWaypointsX[m_LastStart], &xpp[0]);
	double yval = spline_cubic_val ( n, &mT[m_LastStart], ti, &mWaypointsY[m_LastStart], &ypp[0]);
	double zval = spline_cubic_val ( n, &mT[m_LastStart], ti, &mWaypointsZ[m_LastStart], &zpp[0]);
	
	return osg::Vec3(xval, yval, zval);
}

void PathInterpolation::updateWaypointIndex() {
	//while ((mT.size() > 1) && (mWaypointIndex+1 < mT.size()) && (mTi >= mT[mWaypointIndex+1])) {
	while ((mT.size() > 1) && (mWaypointIndex+1 < mT.size()) && (mTi >= mT[mWaypointIndex])) {
		mWaypointIndex++;
	}
}

unsigned int PathInterpolation::getWaypointIndex() {
	updateWaypointIndex();
	return mWaypointIndex;
}

osg::Vec3 PathInterpolation::getTarget(osg::Vec3 pos) {
	if (mT.size() > 1) {
		updateWaypointIndex();
		
		//generate target
		osg::Vec3 targetPt = getInterpolatedPoint(mTi);
		
		//increment index
		double dist = (targetPt - pos).length2(); 
		if (dist < mRange) {
			if ((mTi+mStepSize) < mT[mT.size()-1]) {
				mTi += mStepSize; //next point
			} else {
				mTi = mT[mT.size()-1]; //end point
			}
		}
		return targetPt;
	} else {
		return mLastAddedWaypoint;
	}
}

osg::Vec3 PathInterpolation::getFinalWaypoint() {
	return mLastAddedWaypoint;
}

unsigned int PathInterpolation::getNumOfWaypoints()
  {
    return mWaypointsX.size();
  }
