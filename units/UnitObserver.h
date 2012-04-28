/*
 * UnitObserver.h
 *
 *  Created on: 07-Apr-2009
 *      Author: sbutler
 */

#ifndef UNITOBSERVER_H_
#define UNITOBSERVER_H_

#include "Unit.h"

class Hypothesis;

class UnitObserver: public Unit {
public:
	UnitObserver(UnitObserverType type, const std::string& name = "UnknownObserver",
			osg::Vec3 position = osg::Vec3(0,0,0), int id = -1);
	virtual ~UnitObserver();

	virtual osg::Vec3 getPosition();
	osg::Vec3 getGroundPosition();
	virtual bool setGoalPosition(osg::Vec3 goalPos, bool marker=true);

	virtual void update();
	virtual void updatePhysics();

	void turnTo(double angle);

	float getTimeToTravel(float dist);

	double getAltitude();
	void setAltitude(double);

	double getViewLength();
	double getViewRadius();
	float getViewVariance();
	float getViewResolution();

	UnitObserverType getType();

	virtual double getObservedPositionVariance();

	/**
	 * Set the target to observe
	 */
	//void setTarget(Unit* u, float altitude, float duration);
	void setTarget(Hypothesis* h, UnitGroup* u, float altitude, float duration);
	void setTarget(osg::Vec3 pos, float duration);

	void setIdle();
	bool isIdle();

	float getTargetDuration();

	bool hasArrived();
	void startObservation();

	/**
	 * See if we have arrived at the target for the specified duration
	 */
	bool hasObservedTarget();

	bool hasTargetUnits();
	std::vector<Unit*> getTargetUnits();
	UnitGroup* getTargetGroup();

	Hypothesis* getHypothesis();

	virtual float getMaxSpeed();

protected:
	virtual void LoadMeshes();
	bool isAtTarget();

private:
	UnitObserverType mType;
	float mSpeed;
	float mAltitude;
	float mTurnRate;
	float mStoppingDist;
	float mSlowingDist;
	float mFOV;
	float mResolution;
	Unit* mTargetUnit;
	dtCore::RefPtr<UnitGroup> mTargetGroup;
	float mTargetDuration;
	float mTargetArrivedTime;
	float mMinAltitude;
	bool mArrivedFlag;
	Hypothesis *mHypothesis;
	bool mIdle;
};

#endif /* UNITOBSERVER_H_ */
