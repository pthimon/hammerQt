#ifndef UNITSOLDIER_H_
#define UNITSOLDIER_H_

#ifdef readFloat //HawkNL overwrites this macro that dtChar uses
#undef readFloat
#endif
#ifdef readString //HawkNL overwrites this macro that dtChar uses
#undef readString
#endif
#ifdef writeFloat //HawkNL overwrites this macro that dtChar uses
#undef writeFloat
#endif
#ifdef writeString //HawkNL overwrites this macro that dtChar uses
#undef writeString
#endif
#include <dtAnim/export.h>
#include <dtAnim/animationhelper.h>
#include "../common.h"
#include "UnitFiring.h"

class UnitSoldier : public UnitFiring
{
public:
	UnitSoldier(const std::string& name = "UnknownSoldier", osg::Vec3 position = osg::Vec3(0,0,0),
			double facing = 0, UnitSide side = BLUE, bool controlled = true, int client = -1, int id = -1);
	virtual ~UnitSoldier();
	virtual UnitSoldier* clone();

	//Serialisation
	/*friend class boost::serialization::access;
	template<class Archive>
	void serialize(Archive & ar, const unsigned int version) const
    {
		ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(UnitFiring);
    }*/
	//Serialisation
	friend class boost::serialization::access;
	template<class Archive>
	void save(Archive & ar, const unsigned int version) const
    {
		ar << BOOST_SERIALIZATION_BASE_OBJECT_NVP(UnitFiring);
    }
	template<class Archive>
	void load(Archive & ar, const unsigned int version)
    {
		ar >> BOOST_SERIALIZATION_BASE_OBJECT_NVP(UnitFiring);
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()

    virtual osg::Vec3 getPosition();
    virtual osg::Vec3 getHeadingVector();
	virtual osg::Vec3 getNormal();
	void turnTo(double angle);

	virtual void applyShader();
	virtual void updatePhysics();
	virtual void aim();
	virtual bool isAimed();
	//virtual void freezeAim();
	//virtual void resetAim();
	virtual bool fire();
	virtual osg::Vec3 getProjectileLaunchPos();
	virtual float getMaxSpeed();
	virtual float getSpeed(float x, float y);

protected:
	virtual void LoadMeshes();
	virtual void initPhysics();
	virtual bool applyConfigData(const UnitConfigData& data);
	bool applyUnitSoldierConfigData(const UnitConfigData& data);

private:
	dtCore::RefPtr<dtCore::Object> mSoldier;
	dtCore::RefPtr<dtCore::Object> mMarker;
	dtCore::RefPtr<dtAnim::AnimationHelper> mAnimHelper;
	float mSpeed, mTurnRate, mPrevHeight;
	float mThresholdTurret;
	float mMaxSpeed;
	float mAverageSpeed;
	float mSlowSpeed;
	float mSlowingDist;
	float mStoppingDist;
	float mTurningRate;

};

#endif /*UNITSOLDIER_H_*/
