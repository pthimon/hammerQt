/*
 * Observations.h
 *
 *  Created on: 07-Apr-2009
 *      Author: sbutler
 */

#ifndef OBSERVATIONS_H_
#define OBSERVATIONS_H_

#include <vector>
#include <map>
#include <set>
#include <deque>
#include <QObject>
#include <osg/Vec3>
#include <osg/Referenced>
#include <dtCore/refptr.h>
#include "../../common.h"

class Unit;
class UnitObserver;
class UnitGroup;
class InverseModel;
class Hypothesis;

class ObservationRequest {
public:
	ObservationRequest(Hypothesis* h);
	dtCore::RefPtr<UnitGroup> getNearestNeighbour(osg::Vec3 start, float& distance);
	Hypothesis* getHypothesis();

private:
	Hypothesis* mHypothesis;
	std::vector<dtCore::RefPtr<UnitGroup> > mGroups;
};

class Observations : public QObject {
	Q_OBJECT
public:
	Observations();
	virtual ~Observations();

	void addObserver(UnitObserver* o);
	void removeObserver(UnitObserver* o);
	bool hasObservers();

	void requestObservations(Hypothesis* h);

	void schedule();
	void setTarget(UnitObserver* o);
	void update(float delta);

	enum ScheduleType {
		NONE,
		RR,
		HCAW,
		RR_HCAW,
		RR_HCAW_A
	};

private:
	//std::deque<dtCore::RefPtr<ScheduleItem> > nearestNeighbour(osg::Vec3 start, std::vector<Unit*> requests);
	dtCore::RefPtr<UnitGroup> nearestNeighbour(osg::Vec3 start, std::vector<dtCore::RefPtr<UnitGroup> >& requests);
	osg::Vec3 getObservationPoint(osg::Vec3 startPos, float viewRadius, osg::Vec3 targetPos, float targetRadius);

	//std::map<Unit*,int> mPendingRequests;
	//std::vector<dtCore::RefPtr<UnitGroup> > mPendingRequests;
	std::deque<Hypothesis*> mPendingRequests;
	//std::vector<std::set<Unit*> > mRequests;
	//std::vector<std::deque<dtCore::RefPtr<ScheduleItem> > > mSchedule;
	std::vector<UnitObserver*> mObservers;
	unsigned int mNumObservableGroups;
	unsigned int mNumObservers;
	bool mScheduleFlag;
};

#endif /* OBSERVATIONS_H_ */
