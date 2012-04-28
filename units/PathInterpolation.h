#ifndef PATHINTERPOLATION_H_
#define PATHINTERPOLATION_H_

#include <dtCore/base.h>
#include "spline.h"
#include <osg/Vec3>
#include <vector>

class DT_CORE_EXPORT PathInterpolation : public dtCore::Base                                     
{      
        DECLARE_MANAGEMENT_LAYER(PathInterpolation)
	
public:	
	PathInterpolation(double range = 10.0, double stepSize = 1.0);
	virtual ~PathInterpolation();
	
        void clearWaypoints();
	void addWaypoint(osg::Vec3 p);
	osg::Vec3 getWaypoint(int index);
	osg::Vec3 getTarget(osg::Vec3 pos);
	unsigned int getWaypointIndex();
	osg::Vec3 getFinalWaypoint();
	unsigned int getNumOfWaypoints();
	
private:
	
	void generateSpline(int start);
	osg::Vec3 getInterpolatedPoint(double ti);
	void updateWaypointIndex();
	
	std::vector<double> mWaypointsX;
	std::vector<double> mWaypointsY;
	std::vector<double> mWaypointsZ;
	int mWaypointIndex;
	
	std::vector<double> xpp;
	std::vector<double> ypp;
	std::vector<double> zpp;	
	std::vector<double> mT;
	
	double mTi;
	double mStepSize;
	double mRange;
	
	osg::Vec3 mLastAddedWaypoint, mPrevAddedWaypoint;
	
	int n;
	int m_LastStart;
};

#endif /*PATHINTERPOLATION_H_*/
