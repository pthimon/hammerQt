/*
 * HypothesisManager.h
 *
 *  Created on: 27-Jan-2009
 *      Author: sbutler
 */

#ifndef HYPOTHESISMANAGER_H_
#define HYPOTHESISMANAGER_H_

#include <vector>
#include <map>
#include <deque>
#include <QObject>
#include <osg/Referenced>
#include <osg/Vec3>
#include <dtCore/refptr.h>

#include "../common.h"

class Hypothesis;
class Unit;
class Targets;
class Observations;
class hammerQt;
class HypothesisClient;

class HypothesisManager : public QObject, public osg::Referenced
{
	Q_OBJECT

public:
	/**
	 * Singleton class to manage the hypothesis objects
	 * and to marshal the start and results of predictions
	 * TODO not be singleton
	 */
	HypothesisManager(Targets* targets, Observations* ob, hammerQt* gui);
	virtual ~HypothesisManager();

	static HypothesisManager* create(std::string host, int port, Targets* targets, Observations* ob, hammerQt* gui);
	static HypothesisManager* getManager();
	static HypothesisManager* get();
	static void setHost(std::string host) { mHost = host; }
	static std::string getHost() { return mHost; }
	static void setPort(int port) { mPort = port; }
	static int getPort() { return mPort; }
	static std::vector<dtCore::RefPtr<Hypothesis> >& getHypotheses() { return getManager()->mHypotheses; }

	/**
	 * Setup default target hypotheses. Called by HTAppQT when the hypothesis manager
	 * is created.
	 */
	void initHypotheses();

	void update();

	void startHypothesis(Hypothesis* h, HypothesisClient* c, std::vector<Unit*>& unitsState, int duration);
	void finishHypothesis(Hypothesis* h, std::vector<Unit*>& units);

	void hypothesisFinished(Hypothesis* h);

	double getTimeStep();

	void addHypothesis(Hypothesis*);

	void newPrediction(int clientId, Unit* unit, int total);
	void newState(std::vector<Unit*> &units);

	bool isInitialised() {
		return mInit;
	}
	void setInitialised() {
		mInit = true;
	}

signals:
	void hypothesisUpdate(std::vector<osg::Vec3> params);

private:
	Hypothesis *getNextHypothesis();

	static dtCore::RefPtr<HypothesisManager> mManager;
	static std::string mHost;
	static int mPort;

	Targets* mTargets;
	Observations* mObservations;
	hammerQt* mGUI;

	std::vector<dtCore::RefPtr<Hypothesis> > mHypotheses;
	std::deque<Hypothesis*> mHypothesisQueue;

	std::map<int, std::vector<Unit*> > mPredictedUnits;
	std::map<int, int> mPredictedUnitsCount;

	int mTimeStep;

	bool mInit;
};

#endif /* HYPOTHESISMANAGER_H_ */
