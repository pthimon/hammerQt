#include "DirectionHypothesis.h"

#include <osg/Vec3>
#include <cmath>
#include <sstream>
#include <osg/Math>
#include <osg/Matrix>
#include <limits>
#include <boost/lexical_cast.hpp>

#include "../Command.h"
#include "HypothesisResults.h"
#include "../eventlog.h"

DirectionHypothesis::DirectionHypothesis(std::vector<UnitGroup*>& targetUnits, UnitGroup* myUnits, int numLevels, int numSegments) :
	Hypothesis(targetUnits, myUnits)
{
	mNumSegments = numSegments;
	mThreshold = 0;	//need to have the same segment predicted twice to pass threshold
	mMainSegment = -1;
	mLastIndex = -1;
	mLastIndexCount = 0;
	mDepth = 0;
	mLevels = numLevels;
}

DirectionHypothesis::~DirectionHypothesis()
{
}

void DirectionHypothesis::init() {
	//assign attack commands to each segment
	//each command gets executed separately
	float segment = (2*osg::PI) / mNumSegments;
	osg::Vec3 heading = osg::Vec3(0,100,0);
	calcUnitsAngles(heading);
	for (int i=0; i < mNumSegments; i++) {
		osg::Matrix rotateHeading = osg::Matrix::rotate(segment*i, osg::Vec3(0,0,1));
		osg::Vec3 targetHeading = rotateHeading * heading;
		std::ostringstream oss;
		//oss << "Heading" << i << ">" << targetHeading[0] << ','<< targetHeading[1] << ',' << targetHeading[2];
		//update params
		std::vector<int> ids = mRedUnits->getUnitIDs();
		//get targets
		std::vector<int> targets = getUnitsInSegment(segment*i, segment);
		//oss << "Head>" << segment*i << ",Angle>" << segment << ",";
		oss << "Targets";
		for (unsigned int i=0; i < targets.size(); i++) {
			oss << (theApp->getUnitById(targets[i]))->getName() << ',';
		}
		addCommand(Command::HEADTO, ids, targetHeading, oss.str(), targets);
	}
}

void DirectionHypothesis::update() {
	//called when we have confidences for each separate command
	//update the parameters and the groups
	//save update description in mParamChanges

	/*mAccumulatedConfidences.clear();
	//accumulate average confidence for each command
	for (unsigned int i=0; i < mCommands.size(); i++) {
		mAccumulatedConfidences[i] += mResults[i]->getGroupError(mGroups[i]);
	}*/
	//find the winning segment by seeing if it passes a threshold
	float maxConf = -std::numeric_limits<float>::infinity(); //minus infinity
	int index = 0;
	for (unsigned int i=0; i < mCommands.size(); i++) {
		float conf = mResults[i]->getGroupError(mGroups[i]);
		if (conf > maxConf) {
			maxConf = conf;
			index = i;
		}
	}
	if (mLastIndex == index) {
		//count the number of time maxConf has been in the same index
		mLastIndexCount++;
	} else {
		mLastIndexCount = 0;
	}
	mLastIndex = index;
	int opposite = (int)(mMainSegment+mNumSegments/2)%mNumSegments;
	//bool isodd = (mNumSegments%2 == 1);
	//bool reallocate = false;
	if (mMainSegment == -1) {
		mMainSegment = index;
	} else if (mLastIndexCount >= mThreshold && maxConf > 0.5) {
		if (mDepth < mLevels-1) {
			mDepth++;
		}
		mLastIndexCount = 0;
	} else {
		if (index == opposite || (mNumSegments > 4 && (index == (opposite+1)%mNumSegments || index == (opposite-1)%mNumSegments))) {
			mDepth = 0;
		} else if (mDepth > 0) {
			mDepth--;
		}
	}

		/*if (index == opposite || (mNumSegments > 4 && (index == (opposite+1)%mNumSegments || index == (opposite-1)%mNumSegments))) {
			if (index == opposite) {
				mDepth = 0;
			} else if (mDepth > 0) {
				mDepth--;
			}
			//mAccumulatedConfidences.clear();
		} else if (maxConf > 0.5) {
			if (index == mMainSegment || (mNumSegments > 2 && (index == (mMainSegment+1)%mNumSegments || index == (mMainSegment-1)%mNumSegments))) {
				//mainseg +/-1: narrow the spread a bit
				if (mDepth < mLevels-1) {
					mDepth++;
					reallocate = true;
				}
			} else if (index == opposite || index == (opposite+1)%mNumSegments || index == (opposite-1)%mNumSegments || (isodd && (opposite+2)%mNumSegments)) {
				//opposite of mainseg +/-1: back out as we're getting it wrong
				if (mDepth > 0) {
					if (index == opposite) {
						mDepth = 0;
					} else {
						mDepth--;
					}
					reallocate = true;
				}

			}
			//mAccumulatedConfidences.clear();
		}*/
		//update main segment
		mMainSegment = index;

		std::ostringstream oss;
		oss << "SEG " << index << ", " << mDepth << ": " << mParams.at(mMainSegment);
		EventLog::GetInstance().logEvent(oss.str());

		//do the work
		//if (reallocate) allocateCommands();
	//}
	allocateCommands();
}

float DirectionHypothesis::getAngleFromDepth(int depth) {
	//TODO justify magic number (0.6)
	return (2 * osg::PI * exp( log(0.6) * depth ));
}

void DirectionHypothesis::calcUnitsAngles(osg::Vec3 heading) {
	osg::Vec2 p, v, v90;
	//get starting pos
	osg::Vec3 start = mRedUnits->getCentre();
	//project to 2D plane
	p = osg::Vec2(heading[0], heading[1]);
	p.normalize();

	mAnglesToHeading.clear();
	//get vector to all the enemy (blue) units
	for(unsigned int i=0; i < mBlueUnits.size(); i++) {
		osg::Vec3 cv = mBlueUnits[i]->getCentre() - start;
		v = osg::Vec2(cv[0],cv[1]);
		v.normalize();

		//vector rotated by 90 degrees
		v90 = osg::Vec2(-v[1], v[0]);

		//use dot product formula to get angle (0-PI)
		float vp = v*p;
		float angle = acos(vp);

		//find out whether angle is to the left or right
		float beta = p * v90;

		// actually go left
		if (beta < 0)
			angle = -angle;

		mAnglesToHeading.push_back(angle);
	}
	std::ostringstream oss;
	oss << "ANGL ";
	std::copy(mAnglesToHeading.begin(), mAnglesToHeading.end(), std::ostream_iterator<float>(oss, " "));
	EventLog::GetInstance().logEvent(oss.str());
}

std::vector<int> DirectionHypothesis::getUnitsInSegment(float rotation, float interval) {
	std::vector<int> units;
	if (rotation > M_PI) {
		//stay within +-pi
		rotation -= 2*M_PI;
	} else if (rotation < -M_PI) {
		rotation += 2*M_PI;
	}
	float startAngle = rotation-(interval/2);
	float endAngle = startAngle + interval;
	for(unsigned int i=0; i < mAnglesToHeading.size(); i++) {
		bool found = false;
		float angle = mAnglesToHeading[i];
		//argh stupid annoying wrapping +-pi
		if (endAngle > M_PI) {
			if ((angle > startAngle) || (angle < endAngle-(2*M_PI) )) {
				found = true;
			}
		} else if (startAngle < -M_PI) {
			if ((angle > startAngle+(2*M_PI)) || (angle < endAngle )) {
				found = true;
			}
		} else if((angle > startAngle && angle <= endAngle)) {
			found = true;
		}
		if (found) {
			std::vector<int> ids = mBlueUnits[i]->getUnitIDs();
			units.insert(units.end(), ids.begin(), ids.end());
		}
	}
	return units;
}

void DirectionHypothesis::allocateCommands() {
	//calculate new opposite TODO allow for oddness
	int newopposite = (int)(mMainSegment+mNumSegments/2)%mNumSegments;
	//rotate!
	osg::Vec3 heading = mParams.at(mMainSegment);
	calcUnitsAngles(heading);
	float onesegment = getAngleFromDepth(mDepth) / mNumSegments;
	float mainsegments = onesegment * (mNumSegments-2);
	float oppositesegment = (2*osg::PI - mainsegments)/2;
	osg::Matrix mainangle = osg::Matrix::rotate(onesegment, osg::Vec3(0,0,1));
	osg::Matrix oppositeangle = osg::Matrix::rotate(oppositesegment, osg::Vec3(0,0,1));
	//find the units in this segment and add them to this call
	std::vector<int> targets = getUnitsInSegment(0, onesegment);
	std::ostringstream oss;
	//oss << "Heading" << mMainSegment << ">" << targetHeading[0] << ','<< targetHeading[1] << ',' << targetHeading[2];
	//oss << "Head>0,Angle>" << onesegment << ",";
	oss << "Targets>";
	for (unsigned int i=0; i < targets.size(); i++) {
		oss << (theApp->getUnitById(targets[i]))->getName() << ',';
	}
	//update params
	updateParam(mMainSegment, heading, oss.str(), targets);

	float angle = 0;
	for (int i=1; i < mNumSegments; i++) {
		int seg = (mMainSegment + i)%mNumSegments;
		if (mDepth > 0 && (seg == newopposite || (mNumSegments > 2 && seg == (newopposite+1)%mNumSegments))) {
			heading = oppositeangle * heading;
			angle += oppositesegment;
		} else {
			heading = mainangle * heading;
			angle += onesegment;
		}
		float segmentAngle = onesegment;
		if (seg == newopposite) {
			segmentAngle = (2*osg::PI - (onesegment * (mNumSegments-1)));
		}
		//find the units in this segment and add them to this call
		std::vector<int> targets = getUnitsInSegment(angle, segmentAngle);
		//param string
		std::ostringstream oss;
		//oss << "Heading" << seg << ">" << targetHeading[0] << ','<< targetHeading[1] << ',' << targetHeading[2];
		//oss << "Head>" << angle << ",Angle>" << segmentAngle << ",";
		oss << "Targets>";
		for (unsigned int i=0; i < targets.size(); i++) {
			//oss << ((i>0)?",":"") << (theApp->getUnitById(targets[i]))->getName();
			oss << (theApp->getUnitById(targets[i]))->getName() << ",";
		}
		//update params
		updateParam(seg, heading, oss.str(), targets);
	}
}
