/*
 * Marker.h
 *
 *  Created on: 01-May-2009
 *      Author: sbutler
 */

#ifndef MARKER_H_
#define MARKER_H_

#include <QObject>
#include <QGraphicsItem>
#include <QPainter>
#include <QBrush>
#include <QColor>
#include <QPen>
#include <osg/Vec3>

class Marker : public QGraphicsItem {
public:
	Marker();
	virtual ~Marker();

    virtual QRectF boundingRect() const = 0;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) = 0;

protected:

    float distToPixels(float dist);
    QPointF coordToPoint(float x, float y);
    QPointF coordToPoint(osg::Vec3 pos);

	QColor red;
	QColor lightred;
	QColor blue;
	QColor lightblue;
	QColor grey;
	QColor lightgrey;
};

#endif /* MARKER_H_ */
