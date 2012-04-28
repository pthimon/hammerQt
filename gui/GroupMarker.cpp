/*
 * GroupMarker.cpp
 *
 *  Created on: 01-May-2009
 *      Author: sbutler
 */

#include "GroupMarker.h"

#include "../units/UnitGroup.h"
#include "../units/UnitObserver.h"

GroupMarker::GroupMarker(UnitGroup* group, GroupMarkerType type):
	mGroup(group),
	mType(type)
{
}

GroupMarker::~GroupMarker() {

}


QRectF GroupMarker::boundingRect() const
{
    return QRectF(-(mGroup->getRadiusX()+1),-(mGroup->getRadiusY()+1),
    		2*(mGroup->getRadiusX()+1),2*(mGroup->getRadiusY()+1));
}

void GroupMarker::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
	float x = mGroup->getRadiusX();
	float y = mGroup->getRadiusY();

	QPen pen(QColor(0,0,0,100));

	switch (mType) {
	case GROUP:
		//line
		pen.setStyle(Qt::SolidLine);
		painter->setPen(pen);

		//fill
		painter->setBrush(Qt::NoBrush);

		if (mGroup->getSide() == NONE) {
			//set the observer radius
			UnitObserver *observer = dynamic_cast<UnitObserver*>(mGroup->getUnits()[0]);
			float dist = observer->getViewRadius();
			x = dist;
			y = dist;
			//draw
			painter->drawEllipse(QPointF(0,0),distToPixels(x),distToPixels(y));
		} else {
			//draw
			painter->drawEllipse(QPointF(0,0),distToPixels(x*2),distToPixels(y*2));
		}
		break;
	case OBSERVED_GROUP:
		//line
		painter->setPen(QColor(0,0,0,0));

		//fill
		QRadialGradient radialGrad(QPointF(0, 0), distToPixels(mGroup->getRadius()));

		if (mGroup->getSide() == RED) {
			radialGrad.setColorAt(0, Qt::red);
			radialGrad.setColorAt(0.3, red);
		} else if (mGroup->getSide() == BLUE) {
			radialGrad.setColorAt(0, Qt::blue);
			radialGrad.setColorAt(0.3, blue);
		} else {
			radialGrad.setColorAt(0, Qt::gray);
			radialGrad.setColorAt(0.3, grey);
		}
		radialGrad.setColorAt(1, Qt::transparent);
		painter->setBrush(QBrush(radialGrad));

		//draw
		float dx = distToPixels(x);
		float dy = distToPixels(y);

		painter->drawRect(QRectF(-dx,-dy,2*dx,2*dy));
		break;
	}
}
