#ifndef HYPOTHESISRESULTS_H_
#define HYPOTHESISRESULTS_H_

#include <map>
#include <vector>
#include <string>
#include <Qt>
#include <QAbstractTableModel>
#include <QVariant>
#include <QModelIndex>
#include <QString>
#include <osg/Referenced>
#include <osg/Vec3>
#include <osg/Vec2>
#include <dtCore/refptr.h>

#include "../units/UnitGroup.h"

class HypothesisResult: public osg::Referenced
{
public:
	enum PointsType {
		ABSOLUTE,
		RELATIVE,
		WORLD,
		NONE
	};

	HypothesisResult(std::string name, dtCore::RefPtr<UnitGroup> startUnits, double confidence, double time, std::vector<int> targets) :
		mLabel(name),
		mStartUnits(startUnits),
		mConfidence(confidence),
		mTime(time),
		mPointsType(NONE)
	{
		addTargets(targets);
	}
	void addTarget(dtCore::RefPtr<UnitGroup> targets) {
		mTargetUnits = targets;
	}
	void addTargets(std::vector<int> targets);
	void addPoints(std::vector<osg::Vec2> path, PointsType type = ABSOLUTE) {
		mPoints = path;
		mPointsType = type;
	}
	void setLabel(std::string name) { mLabel = name; }

	std::string getLabel() { return mLabel; }
	double getConfidence() { return mConfidence; }
	double getTime() { return mTime; }
	dtCore::RefPtr<UnitGroup> getStartUnits() { return mStartUnits; }
	dtCore::RefPtr<UnitGroup> getTargetUnits() { return mTargetUnits; }
	std::vector<osg::Vec2> getPoints() { return mPoints; }
	PointsType getPointsType() { return mPointsType; }

private:
	std::string mLabel;
	dtCore::RefPtr<UnitGroup> mStartUnits;
	double mConfidence;
	double mTime;
	dtCore::RefPtr<UnitGroup> mTargetUnits;
	std::vector<osg::Vec2> mPoints;
	PointsType mPointsType;
};

class HypothesisResults : public QAbstractTableModel, public osg::Referenced
{
public:
	HypothesisResults();
	virtual ~HypothesisResults();

	QVariant data ( const QModelIndex & index, int role = Qt::DisplayRole ) const;
	int rowCount ( const QModelIndex & parent = QModelIndex() ) const;
	int columnCount ( const QModelIndex & parent = QModelIndex() ) const;

	QVariant headerData ( int section, Qt::Orientation orientation, int role = Qt::DisplayRole ) const;

	void addResult(int unitId, float result, double timeStep, double duration);
	void addResult(HypothesisResult* r);
	HypothesisResult* getResult();

	std::string getResults();
	float getGroupError(std::vector<int>& group);
	float getGroupError() {
		std::vector<int> group;
		return getGroupError(group);
	}
	float getConfidence();

private:
	//mResults[unitID][index](timeindex) = confidence
	std::map<int, std::vector< std::pair<float,float> > > mResults;
	std::vector<HypothesisResult*> mHypothesisResults;
};

#endif /*HYPOTHESISRESULTS_H_*/
