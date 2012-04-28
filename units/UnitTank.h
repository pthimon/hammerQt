#ifndef UNITTANK_H_
#define UNITTANK_H_

#include "../common.h"
#include "UnitFiring.h"

class UnitTank : public UnitFiring
{
public:
	UnitTank(const std::string& name = "UnknownTank", osg::Vec3 position = osg::Vec3(0,0,0),
		 double facing = 0, UnitSide side = BLUE, bool controlled = true, int client = -1, int id = -1);
	virtual ~UnitTank();
	virtual UnitTank* clone();

	//Serialisation
	friend class boost::serialization::access;
	template<class Archive>
	void saveBody(Archive & ar, const unsigned int version, dtCore::Object* body) const {
		const dReal *pos = dBodyGetPosition(body->GetBodyID());
		const dReal *q = dBodyGetQuaternion(body->GetBodyID());
		const dReal *vel = dBodyGetLinearVel(body->GetBodyID());
		const dReal *avel = dBodyGetAngularVel(body->GetBodyID());
		ar << boost::serialization::make_nvp("m_Position_x", pos[0])
			<< boost::serialization::make_nvp("m_Position_y", pos[1])
			<< boost::serialization::make_nvp("m_Position_z", pos[2]);
		ar << boost::serialization::make_nvp("m_Quat_w", q[0])
			<< boost::serialization::make_nvp("m_Quat_x", q[1])
			<< boost::serialization::make_nvp("m_Quat_y", q[2])
			<< boost::serialization::make_nvp("m_Quat_z", q[3]);
		ar << boost::serialization::make_nvp("m_Vel_x", vel[0])
			<< boost::serialization::make_nvp("m_Vel_y", vel[1])
			<< boost::serialization::make_nvp("m_Vel_z", vel[2]);
		ar << boost::serialization::make_nvp("m_AVel_x", avel[0])
			<< boost::serialization::make_nvp("m_AVel_y", avel[1])
			<< boost::serialization::make_nvp("m_AVel_z", avel[2]);
	}
	template<class Archive>
	void loadBody(Archive & ar, const unsigned int version, dtCore::Object* body) {
		dQuaternion q;
		dReal x,y,z,w;
		ar >> boost::serialization::make_nvp("m_Position_x", x)
			>> boost::serialization::make_nvp("m_Position_y", y)
			>> boost::serialization::make_nvp("m_Position_z", z);
		dBodySetPosition(body->GetBodyID(),x,y,z);
		ar >> boost::serialization::make_nvp("m_Quat_w", w)
			>> boost::serialization::make_nvp("m_Quat_x", x)
			>> boost::serialization::make_nvp("m_Quat_y", y)
			>> boost::serialization::make_nvp("m_Quat_z", z);
		q[0] = w; q[1] = x; q[2] = y; q[3] = z;
		dBodySetQuaternion(body->GetBodyID(),q);
		ar >> boost::serialization::make_nvp("m_Vel_x", x)
			>> boost::serialization::make_nvp("m_Vel_y", y)
			>> boost::serialization::make_nvp("m_Vel_z", z);
		dBodySetLinearVel(body->GetBodyID(),x,y,z);
		ar >> boost::serialization::make_nvp("m_AVel_x", x)
			>> boost::serialization::make_nvp("m_AVel_y", y)
			>> boost::serialization::make_nvp("m_AVel_z", z);
		dBodySetAngularVel(body->GetBodyID(),x,y,z);
	}
	template<class Archive>
	void save(Archive & ar, const unsigned int version) const
    {
		ar << BOOST_SERIALIZATION_BASE_OBJECT_NVP(UnitFiring);

		//save ode body positions/velocities
		saveBody(ar,version,mHull.get());
		//saveBody(ar,version,mTurret.get());
		//saveBody(ar,version,mBarrel.get());
		for (int i = 0; i < 4; i++) {
			saveBody(ar,version,mWheels[i].get());
		}
    }
	template<class Archive>
	void load(Archive & ar, const unsigned int version)
    {
		ar >> BOOST_SERIALIZATION_BASE_OBJECT_NVP(UnitFiring);

		if (!theApp->isAddingPrediction()) {
			//restore ode body velocities
			loadBody(ar,version,mHull.get());
			//loadBody(ar,version,mTurret.get());
			//loadBody(ar,version,mBarrel.get());
			for (int i = 0; i < 4; i++) {
				loadBody(ar,version,mWheels[i].get());
			}
		}
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()

	virtual void updatePhysics();
    virtual osg::Vec3 validatePosition(osg::Vec3 pos);
	virtual void aim();
	virtual bool isAimed();
	virtual void freezeAim();
	virtual void resetAim();
	virtual bool fire();
	virtual osg::Vec3 getProjectileLaunchPos();
	virtual float getMaxSpeed();
	virtual float getSpeed(float x, float y);

protected:
	virtual void LoadMeshes();
	virtual void initPhysics();
	virtual bool applyConfigData(const UnitConfigData& data);
	bool applyUnitTankConfigData(const UnitConfigData& data);

private:
	//TODO generalise to any number of wheels
	dtCore::RefPtr<dtCore::Object> mWheels[4];
	dtCore::RefPtr<dtCore::Object> mTurret;
	dtCore::RefPtr<dtCore::Object> mBarrel;
	dtCore::RefPtr<dtCore::Object> mHull;

	dJointID mJoints[4];
	dJointID mJointTurret;
	dJointID mJointBarrel;

	float mWheelX;
	float mWheelYFront;
	float mWheelYRear;
	float mWheelZ;
	float mWheelScale;
	float mVehicleMass;
	double mWheelMass;
	double mSuspensionERP;
	double mSuspensionCFM;
	double mTurnERP;
	double mTurnCFM;
	float mThresholdTurret;
	float mThresholdBarrel;
	float mMaxSpeed;
	float mAverageSpeed;
	float mSlowSpeed;
	float mSlowingDist;
	float mStoppingDist;
	float mTurningRate;
	float mMaxForce;
};

#endif /*UNITTANK_H_*/
