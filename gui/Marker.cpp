/*
 * Marker.cpp
 *
 *  Created on: 01-May-2009
 *      Author: sbutler
 */

#include "Marker.h"

Marker::Marker():
	red(255,0,0,128),
	lightred(255,0,0,20),
	blue(0,0,255,128),
	lightblue(0,0,255,20),
	grey(0,0,0,128),
	lightgrey(0,0,0,20)
{

}

Marker::~Marker() {
	// TODO Auto-generated destructor stub
}

float Marker::distToPixels(float dist) {
	float mapHorizontalSize = 10000.0;
	float imageSize = 512.0;
	return (dist / mapHorizontalSize) * imageSize;
}

QPointF Marker::coordToPoint(float x, float y) {
	//converts the coordinate system used in the simulator to map image coordinates
	float mapHorizontalSize = 10000.0;
	float imageSize = 512.0;
	return QPointF((x / mapHorizontalSize + 0.5) * imageSize, (-y / mapHorizontalSize + 0.5) * imageSize);
}

QPointF Marker::coordToPoint(osg::Vec3 pos) {
	return coordToPoint(pos.x(),pos.y());
}
