#include "UnitMarker.h"

UnitMarker::UnitMarker(int unitID, UnitSide side):
	mUnitID(unitID),
	mSide(side),
	mAlive(true),
	mCluster(-1),
	mId(-1),
	mVariance(0)
{
}

UnitMarker::~UnitMarker()
{
}

QRectF UnitMarker::boundingRect() const
{
    return QRectF(-mVariance,-mVariance,2*mVariance,2*mVariance);
}

void UnitMarker::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
	if (mAlive) {
		//double d = mVariance;
		if (mSide == RED) {
			painter->setBrush(QBrush(lightred));
		} else if (mSide == BLUE) {
			painter->setBrush(QBrush(lightblue));
		} else {
			painter->setBrush(QBrush(lightgrey));
		}
		painter->setPen(QColor(0,0,0,50));
		//painter->drawEllipse(QPointF(0,0), d, d);

		// marker
		if (mSide == RED) {
			painter->setBrush(QBrush(red));
		} else if (mSide == BLUE) {
			painter->setBrush(QBrush(blue));
		} else {
			painter->setBrush(QBrush(grey));
		}
		//cluster
		if (mCluster >= 0) {
			painter->setPen(QColor((Qt::GlobalColor)(mCluster+8)));
		} else {
			painter->setPen(QColor(Qt::black));
		}

		painter->drawEllipse(-2, -2, 4, 4);
		int i = 8;
		for (std::vector<osg::Vec3>::iterator it = mParams.begin(); it != mParams.end(); it++) {
			painter->setPen(QColor((Qt::GlobalColor)(i)));
			painter->drawLine(QPointF(0,0),QPointF(it->x(),it->y())-pos());
			i++;
		}
	}
	//id
	if (mId >= 0) {
		QFont f;
		f.setPixelSize(5);
		painter->setPen(QColor(Qt::black));
		painter->setFont(f);
		painter->drawText(-2,-5,QString("%1").arg(mId));
	}

    //painter->setBrush(Qt::NoBrush);
}

void UnitMarker::setAlive(bool alive) {
	mAlive = alive;
	update();
}

void UnitMarker::setParams(std::vector<osg::Vec3> params) {
	mParams = params;
	update();
}

void UnitMarker::setCluster(int cluster) {
	mCluster = cluster;
	update();
}

void UnitMarker::setId(int id) {
	mId = id;
	update();
}

void UnitMarker::setVariance(double v) {
	mVariance = v;
	update();
}
