#ifndef TARGETHYPOTHESIS_H_
#define TARGETHYPOTHESIS_H_

#include <Eigen/Core>

#include "Hypothesis.h"

class TargetHypothesis : public Hypothesis
{
	Q_OBJECT
public:
	TargetHypothesis(std::vector<UnitGroup*>& targetUnits, UnitGroup* myUnits);
	virtual ~TargetHypothesis();

	virtual void init();
	virtual void update();

	virtual void saveResults();

	virtual float getDeviation(osg::Vec3 pos);

signals:
	void hypothesisClusterUpdate(std::vector<std::vector<int> > clusters);
	void hypothesisCandidatePath(std::vector<osg::Vec2> paths);
	void hypothesisNewPaths();

protected:
	/**
	 * kmeans clustering
	 * based on kmeans matlab script by Ian T Nabney (1996-2001)
	 *
	 * @param data 	affinity matrix
	 * @oaram ncentres	number of clusters
	 * @return		ordered set of data row indices (the path id) for each cluster
	 * 				(order is based on distance to cluster centre)
	 */
	std::vector<std::vector<int> > kmeans(Eigen::MatrixXd& data, int ncentres);

	/**
	 * Wrapper for the A* path planning algorithm
	 *
	 * @param pos 	world coordinates of starting position
	 * @param goal 	world coordinates of finishing position
	 * @return 		vector of path point in heightmap coordinates
	 */
	std::vector<osg::Vec2> pathToPos(osg::Vec3 pos, osg::Vec3 goal, Unit* unit);

	/**
	 * Compares pointwise the vector differences of two paths, up to the length of
	 * the shortest path
	 *
	 * @param p1	path in heightmap coordinates
	 * @param p2	path in heightmap coordinates
	 * @return		distance metric
	 */
	double comparePaths(std::vector<osg::Vec2> p1, std::vector<osg::Vec2> p2);

	void allocateCommands();

	int mMinNumDims;
	int mMaxNumDims;
	int mNumDims;
	int mNumClusters;
	std::vector<std::vector<int> > mTargets;
	std::map<int, std::vector<osg::Vec2> > mPaths;
	bool mInit;
	float mScale;
	float mSigma;
};

#endif /*TARGETHYPOTHESIS_H_*/
