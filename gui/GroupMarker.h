/*
 * GroupMarker.h
 *
 *  Created on: 01-May-2009
 *      Author: sbutler
 */

#ifndef GROUPMARKER_H_
#define GROUPMARKER_H_

#include "Marker.h"

class UnitGroup;

class GroupMarker : public Marker {
public:
	enum GroupMarkerType { GROUP, OBSERVED_GROUP };

	GroupMarker(UnitGroup* group, GroupMarkerType type);
	virtual ~GroupMarker();

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

protected:
	UnitGroup* mGroup;
	GroupMarkerType mType;
};

#endif /* GROUPMARKER_H_ */
