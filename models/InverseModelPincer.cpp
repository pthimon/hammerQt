/*
 * InverseModelPincer.cpp
 *
 *  Created on: 8 Dec 2009
 *      Author: sbutler
 */

#include "InverseModelPincer.h"

#include "../units/Unit.h"
#include "../units/UnitGroup.h"

#include <Eigen/Core>
#include <Eigen/LU>
#include <clustering/Kmeans.h>

InverseModelPincer::InverseModelPincer(std::vector<Unit*> units):
	mUnits(units)
{
	//assume target has already been chosen
	mTarget = units.at(0)->getGoalPosition();

	//find two groups
	if (!divideGroup()) {
		LOG_ERROR("InverseModelPincer: divideGroup failed to find two groups");
		return;
	}

	//figure out the circle
	osg::Vec3 g1 = mGroup1->getCentre();
	osg::Vec3 g2 = mGroup2->getCentre();
	osg::Vec3 centre;
	osg::Vec2 centre2D;
	float radius = 0;

	//if units start far apart then fit circle
	if ((g1-g2).length2() > 100) {
		//get a circle from the three points
		std::vector<osg::Vec3> points;
		points.push_back(mTarget);
		points.push_back(g1);
		points.push_back(g2);

		radius = findCircle(points, centre2D);
		centre = theApp->mapPointToTerrain(osg::Vec3(centre2D.x(), centre2D.y(), 0));
	}
	//otherwise find centre manually (as half the distance from group 1 to the target)
	if (radius == 0) {
		osg::Vec3 r = (mTarget - g1)/2;
		centre = g1 + r;
		centre2D = osg::Vec2(centre.x(), centre.y());
		radius = r.length();
	}

	//find the angle to the target
	osg::Vec2 target(mTarget.x(), mTarget.y());
	osg::Vec2 ct = target - centre2D;
	ct.normalize();
	osg::Vec2 group1(g1.x(), g1.y());
	osg::Vec2 cg1 = group1 - centre2D;
	cg1.normalize();
	osg::Vec2 group2(g2.x(), g2.y());
	osg::Vec2 cg2 = group2 - centre2D;
	cg2.normalize();
	float g1t = cg1*ct;
	float angle1 = acos(g1t);
	float g2t = cg2*ct;
	float angle2 = acos(g2t);

	//find out whether angle should be CW or CCW
	osg::Vec2 cg1_90(-cg1.y(), cg1.x());
	osg::Vec2 cg2_90(-cg2.y(), cg2.x());
	float beta1 = ct * cg1_90;
	float beta2 = ct * cg2_90;

	//reverse angles if they should go CW as rotations are anti-clockwise
	if (beta1 > 0) {
		// group 1 is going CCW
		if (beta2 > 0) {
			// group 2 should go CW if it is also going CCW
			angle2 = -((2*M_PI)-angle2);
		} else {
			// group 2 goes CCW
			angle2 = -angle2;
		}
	} else {
		// group 1 is going CCW
		angle1 = -angle1;
		// group 2 should go CW if it is also going CCW
		if (beta2 <= 0) {
			angle2 = (2*M_PI)-angle2;
		}
	}

	//create waypoints (rotate cg by i*angle/10 \forall i=1..10)
	int increments = 10;
	float angle1Increment = angle1 / increments;
	float angle2Increment = angle2 / increments;
	//start setting goal position to current position
	mGroup1->setGoalPosition(g1);
	mGroup2->setGoalPosition(g2);
	for (int i=1; i < increments; i++) {
		//group1
		osg::Matrixf rotation1 = osg::Matrixf::rotate(i*angle1Increment, osg::Vec3(0,0,1));
		osg::Vec3 ci1 = (g1-centre) * rotation1;
		mGroup1->addGoalPosition(theApp->mapPointToTerrain(ci1 + centre));
		//group2
		osg::Matrixf rotation2 = osg::Matrixf::rotate(i*angle2Increment, osg::Vec3(0,0,1));
		osg::Vec3 ci2 = (g2-centre) * rotation2;
		mGroup2->addGoalPosition(theApp->mapPointToTerrain(ci2 + centre));
	}
}

InverseModelPincer::~InverseModelPincer() {
}

void InverseModelPincer::update() {
	//maybe do something here to keep them at the same absolute angle around the circle,
	//so they arrive at the same time...
}

bool InverseModelPincer::divideGroup() {
	//kmeans, k=2
	unsigned int numGroups = 2;
	unsigned int size = mUnits.size();
	//make sure we can actually make the groups
	if (size < numGroups) return false;
	//make affinity matrix
	Eigen::MatrixXd m = Eigen::MatrixXd::Zero(size,size);
	for (unsigned int i=0; i < size; i++) {
		for (unsigned int j=i+1; j < size; j++) {
			//get euclidian distance (squared) between the points
			float dist = (mUnits.at(i)->getPosition() - mUnits.at(j)->getPosition()).length2();
			//scaling
			double cp = exp(dist/100);
			m(i,j) = cp;
			m(j,i) = cp;
		}
	}
	//cluster
	std::vector<std::vector<int> > clusters = Kmeans::cluster(m, numGroups);
	if (clusters.size() != 2) return false;
	//assign units (from ids) to groups
	mGroup1 = new UnitGroup(RED);
	std::vector<Unit*> units;
	//std::cout << "Group 1: ";
	for (std::vector<int>::iterator it = clusters.at(0).begin(); it != clusters.at(0).end(); it++) {
		units.push_back(mUnits.at(*it));
		//std::cout << *it << " ";
	}
	mGroup1->addUnits(units);

	mGroup2 = new UnitGroup(RED);
	units.clear();
	//std::cout << std::endl << "Group 2: ";
	for (std::vector<int>::iterator it = clusters.at(1).begin(); it != clusters.at(1).end(); it++) {
		units.push_back(mUnits.at(*it));
		//std::cout << *it << " ";
	}
	//std::cout << std::endl;
	mGroup2->addUnits(units);

	return true;
}

/**
 * Algorithm to find the centre and radius of a circle from three points
 * http://home.att.net/~srschmitt/zenosamples/zs_circle3pts.html
 *
 * @param p vector of 3 points of the circle
 * @param c returns the 2D centre of the circle
 * @returns radius
 */
float InverseModelPincer::findCircle(std::vector<osg::Vec3> p, osg::Vec2 &c) {
	int i;
	float r, m11, m12, m13, m14;
	Eigen::Matrix3f a;

	for (i = 0; i < 3; i++) { // find minor 11
		a(i, 0) = p[i].x();
		a(i, 1) = p[i].y();
		a(i, 2) = 1;
	}
	m11 = a.determinant();

	for (i = 0; i < 3; i++) { // find minor 12
		a(i, 0) = pow(p[i].x(), 2) + pow(p[i].y(), 2);
		a(i, 1) = p[i].y();
		a(i, 2) = 1;
	}
	m12 = a.determinant();

	for (i = 0; i < 3; i++) { // find minor 13
		a(i, 0) = pow(p[i].x(), 2) + pow(p[i].y(), 2);
		a(i, 1) = p[i].x();
		a(i, 2) = 1;
	}
	m13 = a.determinant();

	for (i = 0; i < 3; i++) { // find minor 14
		a(i, 0) = pow(p[i].x(), 2) + pow(p[i].y(), 2);
		a(i, 1) = p[i].x();
		a(i, 2) = p[i].y();
	}
	m14 = a.determinant();

	if (m11 == 0)
		r = 0; // not a circle
	else {
		c.x() = 0.5 * m12 / m11; // centre of circle
		c.y() = -0.5 * m13 / m11;
		r = sqrt(pow(c.x(), 2) + pow(c.y(), 2) + m14 / m11);
	}

	return r; // the radius
}
