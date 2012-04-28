#ifndef UNITMARKER_H_
#define UNITMARKER_H_

#include "Marker.h"
#include "../units/Unit.h"

class UnitMarker : public Marker
{
public:
	UnitMarker(int unitID, UnitSide side);
	virtual ~UnitMarker();

    QRectF boundingRect() const;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
    void setAlive(bool alive);
    UnitSide getSide() { return mSide; }
    void setParams(std::vector<osg::Vec3> params);
    void setCluster(int cluster);
    void setId(int id);
    void setVariance(double v);

protected:
	int mUnitID;
	UnitSide mSide;
	bool mAlive;
	std::vector<osg::Vec3> mParams;
	int mCluster;
	int mId;
	double mVariance;
};

#endif /*UNITMARKER_H_*/
