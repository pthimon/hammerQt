#include "HypothesisResults.h"
#include "../common.h"
#include "../HTAppBase.h"
#include "../units/Unit.h"

void HypothesisResult::addTargets(std::vector<int> targets) {
	UnitGroup* g = new UnitGroup(BLUE);
	for (unsigned int i=0; i < targets.size(); i++) {
		g->addUnit(theApp->getUnitById(targets[i]));
	}
	mTargetUnits = g;
}

HypothesisResults::HypothesisResults() : QAbstractTableModel()
{
	mResults.clear();
}

HypothesisResults::~HypothesisResults()
{
}

QVariant HypothesisResults::data ( const QModelIndex & index, int role) const {
	if (!index.isValid())
		return QVariant();

	if (index.row() >= rowCount())
		return QVariant();
	if (index.column() >= columnCount())
		return QVariant();

	if (role == Qt::DisplayRole)
		return mResults.find(index.column())->second[index.row()].second;
	else
		return QVariant();
}

int HypothesisResults::rowCount ( const QModelIndex & parent) const {
	return (mResults.begin() != mResults.end()) ? mResults.begin()->second.size() : 0;
}

int HypothesisResults::columnCount ( const QModelIndex & parent) const {
	return mResults.size();
}

QVariant HypothesisResults::headerData ( int section, Qt::Orientation orientation, int role) const {
	if (role != Qt::DisplayRole)
		return QVariant();

	if (orientation == Qt::Horizontal)
		return QString("%1").arg(QString(theApp->getUnitById(section)->GetName().c_str()));
	else if (mResults.begin() != mResults.end())
		return QString("%1").arg(mResults.begin()->second[section].first);

	return QVariant();
}

void HypothesisResults::addResult(int unitId, float result, double timeStep, double duration) {
	mResults[unitId].push_back(std::pair<float,float>((float)(timeStep+duration),result));
	emit reset();
}

void HypothesisResults::addResult(HypothesisResult* r) {
	//add a result object for each time the hypothesis command is run
	mHypothesisResults.push_back(r);
}

/**
 * Gets the latest result
 */
HypothesisResult* HypothesisResults::getResult() {
	if (!mHypothesisResults.empty()) {
		return mHypothesisResults.back();
	}
	return NULL;
}

/**
 * Gets all of the results formatted as a log file string
 */
std::string HypothesisResults::getResults() {
	std::ostringstream oss;
	std::map<int, std::vector<std::pair<float,float> > >::iterator it;
	it = mResults.begin();
	unsigned int numResults = (it != mResults.end()) ? it->second.size() : 0;
	oss << "Length\t" << numResults << "\t"  << std::endl;
	//print headers for the confidence data
	oss << "TimeStep\t";
	for (it = mResults.begin(); it != mResults.end(); it++) {
		oss << theApp->getUnitName(it->first) << "\t";
	}
	oss << std::endl;
	//data
	for (unsigned int i=0; i < numResults; i++) {
		if (mResults.begin() != mResults.end()) {
			//print time step
			oss << mResults.begin()->second[i].first << "\t";
			//loop through units
			for (it = mResults.begin(); it != mResults.end(); it++) {
				//print confidence for the ith row, if it exists
				if (i < it->second.size()) {
					oss << it->second[i].second << "\t";
				} else {
					oss << "0\t";
				}
			}
			oss << std::endl;
		}
	}
	oss << std::endl;

	return oss.str();
}

/**
 * Gets the latest confidence of the results (of the hypothesis command)
 * Averaged over a time window
 * Will decay confidence if called on a timestep without a result
 */
float HypothesisResults::getConfidence() {
	//get average error over window size for enemy units
	float windowSize = 20;
	std::map<int, std::vector<std::pair<float,float> > >::iterator it;
	int count = 0;
	float acc = 0;
	float average = 0;
	float lastSampleTime = 0;
	if (!mResults.empty()) {
		//loop through units
		for (it = mResults.begin(); it != mResults.end(); it++) {
			//if is an enemy
			Unit* u = theApp->getUnitById(it->first);
			if (u && u->getSide() == RED) {
				//if not empty
				if (!it->second.empty()) {
					//get average error
					float timeIndex = (it->second.size()-1);
					float endTime = it->second[timeIndex].first;
					float winStartTime = endTime - windowSize;
					if (endTime > lastSampleTime) {
						lastSampleTime = endTime;
					}
					int winIndex = 0;
					float accWindow = 0;
					//accumulate all the samples backwards until winStartTime
					while ((timeIndex - winIndex) >= 0 && it->second[timeIndex-winIndex].first > winStartTime) {
						accWindow += it->second[timeIndex-winIndex].second;
						winIndex++;
					}
					acc += accWindow / winIndex;
					count++;
				}
			}

		}
		//average of the units in the group
		average = acc / count;
	}
	//decay with time
	double time = theApp->getSimTime();
	double timeDiff = time - lastSampleTime;
	double decayTime = windowSize - timeDiff;
	if (decayTime < 0) decayTime = 0;
	float decayRatio = decayTime / windowSize;
	return average * decayRatio;
}

/**
 * Gets the latest average result for the group
 */
float HypothesisResults::getGroupError(std::vector<int>& group) {
	//mResults[unitId][index](timeIndex) = confidence
	std::map<int, std::vector<std::pair<float,float> > >::iterator it;
	int count = 0;
	float acc = 0;
	float average = 0;
	if (!mResults.empty()) {
		//loop through units
		for (it = mResults.begin(); it != mResults.end(); it++) {
			//if no group specified then add all to average...
			bool found = true;
			if (!group.empty()) {
				//...otherwise only if unit is in group then add to average
				found = false;
				for (unsigned int i=0; i < group.size(); i++) {
					if (it->first == group[i]) {
						found = true;
						break;
					}
				}
			}
			if (found) {
				//if not empty
				if (!it->second.empty()) {
					//get latest confidence
					acc += (it->second.end()-1)->second;
					count++;
				}
			}

		}
		average = acc / count;
	}
	return average;
}
