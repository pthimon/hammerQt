#include "TargetHypothesis.h"

#include <osg/Vec3>
#include <cmath>
#include <sstream>
#include <osg/Math>
#include <osg/Matrix>
#include <limits>
#include <Eigen/Core>
#include <Eigen/QR>
#include <Eigen/Array>
#include <time.h>
#include <string>
#include <boost/lexical_cast.hpp>

#include "../Command.h"
#include "HypothesisResults.h"
#include "../eventlog.h"
#include <clustering/SpectralClustering.h>

TargetHypothesis::TargetHypothesis(std::vector<UnitGroup*>& targetUnits, UnitGroup* myUnits) :
	Hypothesis(targetUnits, myUnits),
	mMinNumDims(2),
	mMaxNumDims(10),
	mNumDims(0),
	mNumClusters(0),
	mInit(false)
{
	//path distance metric parameter
	mScale = 10;
	//spectral clustering parameter
	float sigma = 10;
	mSigma = sigma*sigma;
}

TargetHypothesis::~TargetHypothesis()
{
}

void TargetHypothesis::init() {
	/*allocateCommands();

	for (int i=0; i < mNumClusters; i++) {
		osg::Vec3 targetPos = theApp->getUnitById(mTargets[i][0])->getPosition();
		std::ostringstream oss;
		//oss << "Target" << i << ">" << targetPos[0] << ','<< targetPos[1] << ',' << targetPos[2];
		oss << "Cluster" << i << ">";
		for (unsigned int j=0; j<mTargets[i].size(); j++) {
			oss << ((j!=0)?",":"") << mTargets[i][j];
		}
		std::cout << "Target " << mTargets[i][0] << ", " << targetPos << std::endl;
		//update params
		std::vector<int> ids = mRedUnits->getUnitIDs();
		addCommand(Command::GOTO, ids, targetPos, oss.str(), mTargets[i]);
	}*/
	//std::vector<int> ids = mRedUnits->getUnitIDs();
	//initCommands(mNumClusters, (int)Command::GOTO, ids);
}

void TargetHypothesis::update() {
	//called when we have confidences for each separate command
	//update the parameters and the groups

	allocateCommands();

	int numClusters = mNumClusters;

	//clearCommands();

	if (!mInit) {
		std::vector<int> ids = mRedUnits->getUnitIDs();
		initCommands(mNumDims, (int)Command::PATHTO, ids);
		mInit = true;
	}

	for (int i=0; i < numClusters; i++) {
		if (mTargets.size() == i) {
			//found less clusters than specified
			break;
		}
		if (mTargets[i].empty()) {
			//for some reason there is an empty cluster, so skip it
			numClusters++;
			continue;
		}
		Unit* u = theApp->getUnitById(mTargets[i][0]);
		if (u) {
			osg::Vec3 targetPos = u->getPosition();
			std::ostringstream oss;
			//oss << "Target" << i << ">" << targetPos[0] << ','<< targetPos[1] << ',' << targetPos[2];
			oss << "Cluster" << i << ">";
			for (unsigned int j=0; j<mTargets[i].size(); j++) {
				oss << ((j!=0)?",":"") << mTargets[i][j];
			}
			std::cout << "Target " << mTargets[i][0] << ", " << targetPos << std::endl;
			//update params
			updateParam(i, targetPos, oss.str(), mTargets[i]);
		}
	}
	for (int i=numClusters; i < mNumDims; i++) {
		//disable the commands we're not using this time
		disableCommand(i);
	}
}

/**
 * Update mTargets with the ids of the units to head towards
 */
void TargetHypothesis::allocateCommands() {
	mTargets.clear();

	//std::map<int, std::vector<osg::Vec2> > paths;

	//--- calc path to each enemy ---

	//get starting point
	//TODO don't just go from centre
	osg::Vec3 start = mRedUnits->getCentre();

	//unsigned int maxlen = 0;
	mPaths.clear();
	emit hypothesisNewPaths();
	for(unsigned int i=0; i < mBlueUnits.size(); i++) {
		//for all the units in the group
		std::vector<Unit*> units = mBlueUnits[i]->getUnits();
		for(unsigned int j=0; j < units.size(); j++) {
			//targets
			int id = units[j]->getUnitID();
			std::cout << "Creating path to " << id << std::endl;
			//HACK: last param is to get a unit type out of a unit group
			std::vector<osg::Vec2> p = pathToPos(start,units[j]->getPosition(), mRedUnits->getUnits()[0]);
			if (!p.empty()) {
				mPaths[id] = p;
				//if (paths[i].size() > maxlen) maxlen = paths[i].size();
				emit hypothesisCandidatePath(mPaths[id]);
			}
		}
	}

	//the number of dimensions to perform the clustering in
	//i.e. for spectral clustering, the number of largest eigenvectors to consider
	//mNumDims = mPaths.size() - 1;
	mNumDims = mPaths.size();
	//limit the number of dimensions to a max and min
	if (mNumDims == 0) {
		//no paths found, so dont cluster
		for(unsigned int i=0; i < mBlueUnits.size(); i++) {
			std::vector<Unit*> units = mBlueUnits[i]->getUnits();
			for(unsigned int j=0; j < units.size(); j++) {
				std::vector<int> c;
				c.push_back(units[j]->getUnitID());
				mTargets.push_back(c);
			}
		}
		mNumClusters = mTargets.size();
		return;
	} else if (mNumDims < mMinNumDims) {
		mNumDims = mMinNumDims;
	} else if (mNumDims > mMaxNumDims) {
		mNumDims = mMaxNumDims;
	}

	//export the paths to plot in matlab
	/*std::cout << "X Paths" << std::endl;
	std::map<int, std::vector<osg::Vec2> >::iterator itp1;
	for(itp1 = paths[*itb].begin(); itp1 != paths[*itb].end(); itp1++) {
		for (unsigned int j=0; j < maxlen; j++) {
			if (j < itp1->second.size())
				std::cout << itp1->second[j].x() << " ";
			else
				std::cout << "NAN ";
		}
		std::cout << std::endl;
	}
	std::cout << "Y Paths" << std::endl;
	for(itp1 = paths[*itb].begin(); itp1 != paths[*itb].end(); itp1++) {
		for (unsigned int j=0; j < maxlen; j++) {
			if (j < itp1->second.size())
				std::cout << itp1->second[j].y() << " ";
			else
				std::cout << "NAN ";
		}
		std::cout << std::endl;
	}*/


	//--- calc affinity matrix ---

	std::vector<int> ids;
	//initialise matrix to zeros (matrix dimensions is number of paths)
	unsigned int size = mPaths.size();
	Eigen::MatrixXd m = Eigen::MatrixXd::Zero(size,size);
	//do the path comparison (top right triangle of affinity matrix)
	unsigned int i=0;
	std::map<int, std::vector<osg::Vec2> >::iterator itp;
	std::map<int, std::vector<osg::Vec2> >::iterator end = mPaths.end();
	end--;
	for(itp = mPaths.begin(); itp != end; itp++) {
		if (itp->second.size() > 0) {
			unsigned int j=i+1;
			std::map<int, std::vector<osg::Vec2> >::iterator start = itp;
			start++;
			for(std::map<int, std::vector<osg::Vec2> >::iterator itp2 = start; itp2 != mPaths.end(); itp2++) {
				//check paths exists
				if (itp->second.size() < 1 && itp2->second.size() < 1) continue;
				//compare
				double cp = comparePaths(itp->second, itp2->second);
				//scaling
				cp = exp(-cp/mSigma);
				//similarity matrix
				m(i,j) = cp;
				m(j,i) = cp;
				j++;
			}
			i++;
			//save the index of the unit id
			ids.push_back(itp->first);
		}
	}
	//final row
	ids.push_back(itp->first);

	//-- Spectral clustering --

	std::vector<std::vector<int> > clusters;

	if (m.rows() > mMinNumDims) {
		std::clock_t startTime = clock();

		SpectralClustering* c = new SpectralClustering(m, mNumDims);
		//clusters = c->clusterKmeans(mNumClusters);
		clusters = c->clusterRotate();

		std::cout << "Clustering took " << (double)(clock() - startTime)/(double)CLOCKS_PER_SEC << " secs" << std::endl;
	} else {
		//just put each unit in a different cluster
		for (int i=0; i < mPaths.size(); i++) {
			std::vector<int> c;
			c.push_back(i);
			clusters.push_back(c);
		}
	}

	//--- show the cluster each unit is assigned to ---

	for (unsigned int i=0; i < clusters.size(); i++) {
		std::cout << "Cluster " << i << ": " << "Unit ";
		for (unsigned int j=0; j < clusters[i].size(); j++) {
			std::cout << ((j!=0)?",":"") << ids[clusters[i][j]];
			//convert path matrix ids to unit ids
			clusters[i][j] = ids[clusters[i][j]];
		}
		if (clusters[i].size() > 0) {
			mTargets.push_back(clusters[i]);
			for (unsigned int j=0; j < clusters[i].size(); j++) {
				//save candidate path (the first in the cluster)
				std::ostringstream oss;
				oss << "PATH " << getID() << " " << i << " " << clusters[i][j] << " ";
				for (unsigned int k=0; k < mPaths[clusters[i][j]].size(); k++) {
					oss << mPaths[clusters[i][j]][k] << "; ";
				}
				EventLog::GetInstance().logEvent(oss.str());
			}
		}
		std::cout << std::endl;
	}
	//Record the number of clusters we've found
	mNumClusters = mTargets.size();

	//save paths
	//emit hypothesisClusterUpdate(clusters);
}

/**
 * Wrapper for the A* path planning algorithm
 *
 * @param pos 	world coordinates of starting position
 * @param goal 	world coordinates of finishing position
 * @param unit	used to get the speed of the unit on different terrain for the A* algorithm
 * @return 		vector of path point in heightmap coordinates
 */
std::vector<osg::Vec2> TargetHypothesis::pathToPos(osg::Vec3 pos, osg::Vec3 goal, Unit* unit) {
	//path is in heightmap coordinates
	std::vector< osg::Vec2 > path;
	float totalCost;

	int result = theApp->getTerrainMap()->solve(pos, goal, path, totalCost, unit);

	std::cout << "Cost: "<<totalCost<<std::endl;

	if (result != micropather::MicroPather::SOLVED) {
		LOG_WARNING("Path not found");
	}
	return path;
}

/**
 * Compares pointwise the vector differences of two paths, up to the length of
 * the shortest path
 *
 * @param p1	path in heightmap coordinates
 * @param p2	path in heightmap coordinates
 * @return		distance metric
 */
double TargetHypothesis::comparePaths(std::vector<osg::Vec2> p1, std::vector<osg::Vec2> p2) {
	if (p1.size() < 1 && p2.size() < 1) {
		LOG_WARNING("Paths are empty")
		return 0;
	}
	//return variable
	double difference = 0;
	//compare up to the smallest path size
	unsigned int len = (p1.size() > p2.size()) ? p2.size() : p1.size();

	for (unsigned int i=1; i < len; i++) {
		//vector offsets to previous point
		osg::Vec2 v1 = p1[i] - p1[i-1];
		osg::Vec2 v2 = p2[i] - p2[i-1];
		//magnitude of difference between vectors scaled by distance along path
		double scale = mScale/(i);
		difference += (v2 - v1).length2() * scale;
	}
	return difference;
}

void TargetHypothesis::saveResults() {
	Hypothesis::saveResults();

	for (unsigned int i=0; i < mTargets.size(); i++) {
		HypothesisResult* r = mResults[i]->getResult();
		r->setLabel(getName() + "Target" + boost::lexical_cast<std::string>(i));
		if (!mTargets[i].empty()) {
			r->addPoints(mPaths[mTargets[i][0]]);
			dtCore::RefPtr<UnitGroup> g = new UnitGroup(BLUE);
			for (unsigned int j=0; j < mTargets[i].size(); j++) {
				g->addUnit(theApp->getUnitById(mTargets[i][j]));
			}
			r->addTarget(g);
		}
	}

	emit hypothesisClusterUpdate(mTargets);
}

/**
 * Gets the smallest deviation from the point to any of the points on the predicted path
 * in 2D
 */
float TargetHypothesis::getDeviation(osg::Vec3 pos) {
	//get the predicted path
	/*HypothesisResults *results = getWinningCommandResults();
	if (!results) {
		EventLog::GetInstance().logEvent("HYPX no winning command");
		return -1;
	}
	HypothesisResult *result = results->getResult();
	if (!result) {
		EventLog::GetInstance().logEvent("HYPX no results from command");
		return -1;
	}
	std::vector<osg::Vec2> path = result->getPoints();
	if (path.size() == 0) {
		EventLog::GetInstance().logEvent("HYPX no path");
		return -1;
	}

	//convert path to world coords
	for (std::vector<osg::Vec2>::iterator it = path.begin(); it != path.end(); it++) {
		*it = theApp->getTerrainMap()->mapToWorldCoords(*it);
	}

	osg::Vec2 point = osg::Vec2(pos.x(), pos.y());
	if (path.size() == 1) {
		//easy, the path only has one point
		return (point - *(path.begin())).length();
	}
	float deviation = std::numeric_limits<float>::max();
	for (std::vector<osg::Vec2>::iterator it = path.begin(); it != path.end(); /*it++*) {
		//get start and end of line segment (also incrementing iterator)
		osg::Vec2 start = *it;
		if (++it == path.end()) break;
		osg::Vec2 end = *it;

		//find deviation perpendicular to the line from start to end (see http://local.wasp.uwa.edu.au/~pbourke/geometry/pointline/)
		osg::Vec2 line = end - start;
		//get proportion of line where deviation meets it (u = 0-1)
		float u = ((point.x() - start.x())*(end.x()-start.x()) + (point.y() - start.y())*(end.y()-start.y())) / line.length2();
		//see if we are in line segment
		float currDeviation = std::numeric_limits<float>::max();
		if (u < 0) {
			currDeviation = (point - start).length2();
		} else if (u > 1) {
			currDeviation = (point - end).length2();
		} else {
			//calc length
			currDeviation = ((start + (line * u))-point).length2();
		}

		//float currDeviation = (point - *it).length2();
		//find the smallest deviation
		if (currDeviation < deviation) {
			deviation = currDeviation;
		}
	}
	return sqrt(deviation);*/

	//get the target units
	std::vector<int> targetUnits = getTargetUnits();
	if (targetUnits.size() <= 0) {
		EventLog::GetInstance().logEvent("HYPX no winning targets");
		return -1;
	}
	int unit = targetUnits[0];

	//get candidate target
	int target = -1;
	for (unsigned int i=0; i < mTargets.size(); i++) {
		for (unsigned int j=0; j < mTargets[i].size(); j++) {
			if (mTargets[i][j] == unit) {
				target = mTargets[i][0];
			}
		}
	}
	if (target == -1) {
		EventLog::GetInstance().logEvent("HYPX no target found!");
	}

	/*std::ostringstream oss;
	oss << "UNIT " << getID() << " " << target << " " << theApp->getUnitById(target)->getName();
	EventLog::GetInstance().logEvent(oss.str());*/

	std::vector<osg::Vec2> path = mPaths[target];
	if (path.size() == 0) {
		EventLog::GetInstance().logEvent("HYPX no path");
		return -1;
	}

	//convert path to world coords
	for (std::vector<osg::Vec2>::iterator it = path.begin(); it != path.end(); it++) {
		*it = theApp->getTerrainMap()->mapToWorldCoords(*it);
	}

	osg::Vec2 point = osg::Vec2(pos.x(), pos.y());
	if (path.size() == 1) {
		//easy, the path only has one point
		return (point - *(path.begin())).length();
	}
	float deviation = std::numeric_limits<float>::max();
	for (std::vector<osg::Vec2>::iterator it = path.begin(); it != path.end(); /*it++*/) {
		//get start and end of line segment (also incrementing iterator)
		osg::Vec2 start = *it;
		if (++it == path.end()) break;
		osg::Vec2 end = *it;

		//find deviation perpendicular to the line from start to end (see http://local.wasp.uwa.edu.au/~pbourke/geometry/pointline/)
		osg::Vec2 line = end - start;
		//get proportion of line where deviation meets it (u = 0-1)
		float u = ((point.x() - start.x())*(end.x()-start.x()) + (point.y() - start.y())*(end.y()-start.y())) / line.length2();
		//see if we are in line segment
		float currDeviation = std::numeric_limits<float>::max();
		if (u < 0) {
			currDeviation = (point - start).length2();
		} else if (u > 1) {
			currDeviation = (point - end).length2();
		} else {
			//calc length
			currDeviation = ((start + (line * u))-point).length2();
		}

		//float currDeviation = (point - *it).length2();
		//find the smallest deviation
		if (currDeviation < deviation) {
			deviation = currDeviation;
		}
	}
	return sqrt(deviation);
}

