#ifndef PROJECTILE_H_
#define PROJECTILE_H_

#include "../common.h"
#include "Unit.h"
#include <dtCore/object.h>
#include <dtCore/particlesystem.h>
#include <ode/ode.h>
#include <osg/Vec3>
#include <string>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/nvp.hpp>

//class Unit;

class Projectile : public dtCore::Object
{
public:
	enum ProjectileType {
		SHELL, BULLET
	};

	Projectile(float radius = 0, float mass = 0, float yield = 0, ProjectileType type = SHELL);
	virtual ~Projectile();
	
	//Serialisation
	friend class boost::serialization::access;
	template<class Archive>
	void saveBody(Archive & ar, const unsigned int version, dBodyID body) const {
		const dReal *pos = dBodyGetPosition(body);
		const dReal *q = dBodyGetQuaternion(body);
		const dReal *vel = dBodyGetLinearVel(body);
		const dReal *avel = dBodyGetAngularVel(body);
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
	void loadBody(Archive & ar, const unsigned int version, dBodyID body) {
		dQuaternion q;
		dReal x,y,z,w;
		ar >> boost::serialization::make_nvp("m_Position_x", x) 
			>> boost::serialization::make_nvp("m_Position_y", y) 
			>> boost::serialization::make_nvp("m_Position_z", z);
		dBodySetPosition(body,x,y,z);
		ar >> boost::serialization::make_nvp("m_Quat_w", w) 
			>> boost::serialization::make_nvp("m_Quat_x", x) 
			>> boost::serialization::make_nvp("m_Quat_y", y) 
			>> boost::serialization::make_nvp("m_Quat_z", z);
		q[0] = w; q[1] = x; q[2] = y; q[3] = z;
		dBodySetQuaternion(body,q);
		ar >> boost::serialization::make_nvp("m_Vel_x", x) 
			>> boost::serialization::make_nvp("m_Vel_y", y) 
			>> boost::serialization::make_nvp("m_Vel_z", z);
		dBodySetLinearVel(body,x,y,z);
		ar >> boost::serialization::make_nvp("m_AVel_x", x) 
			>> boost::serialization::make_nvp("m_AVel_y", y) 
			>> boost::serialization::make_nvp("m_AVel_z", z);
		dBodySetAngularVel(body,x,y,z);
	}
	template<class Archive>
	void save(Archive & ar, const unsigned int version) const
    {
		//projectile constructor properties
        ar & BOOST_SERIALIZATION_NVP(radius);
        ar & BOOST_SERIALIZATION_NVP(mass);
        ar & BOOST_SERIALIZATION_NVP(mYield);
        ar & BOOST_SERIALIZATION_NVP(mType);
        
        //variables
        ar & BOOST_SERIALIZATION_NVP(armed);
        const Unit* const unit = const_cast<Projectile*>(this)->mOwner.get();
        ar & boost::serialization::make_nvp("mOwner", unit);
        osg::Vec3 p = target;
        ar & boost::serialization::make_nvp("target_x", p[0]) 
        	& boost::serialization::make_nvp("target_y", p[1]) 
        	& boost::serialization::make_nvp("target_z", p[2]);
        
        //position
        /*osg::Vec3 center = const_cast<Projectile*>(this)->getPosition();	
    	ar & boost::serialization::make_nvp("pos_x", center[0]) 
	    	& boost::serialization::make_nvp("pos_y", center[1]) 
	    	& boost::serialization::make_nvp("pos_z", center[2]);*/
        
        //ode body position/forces
        saveBody(ar, version, GetBodyID());
    }
	template<class Archive>
	void load(Archive & ar, const unsigned int version)
    {
		//projectile constructor properties
        ar & BOOST_SERIALIZATION_NVP(radius);
        ar & BOOST_SERIALIZATION_NVP(mass);
        ar & BOOST_SERIALIZATION_NVP(mYield);
        ar & BOOST_SERIALIZATION_NVP(mType);
        
        //variables
        ar & BOOST_SERIALIZATION_NVP(armed);
        Unit *owner;
        ar & boost::serialization::make_nvp("mOwner",owner);
        mOwner = dtCore::RefPtr<Unit>(owner);
        ar & boost::serialization::make_nvp("target_x", target[0]) 
        	& boost::serialization::make_nvp("target_y", target[1]) 
        	& boost::serialization::make_nvp("target_z", target[2]);
        
        //add unit to scene and initialize physics
        init();
        
        //position
        /*osg::Vec3 center;
        ar & boost::serialization::make_nvp("pos_x", center[0]) 
	    	& boost::serialization::make_nvp("pos_y", center[1]) 
	    	& boost::serialization::make_nvp("pos_z", center[2]);
        setPosition(center);*/
        
        loadBody(ar, version, GetBodyID());
        PostPhysicsStepUpdate();
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
	
    void init();
	void initPhysics();
	
	float getMass() { return mass; }
	float getRadius() { return radius; }
	
	void setPosition(osg::Vec3 pos);
	osg::Vec3 getPosition();
	
	void setTarget(osg::Vec3 target) { this->target = target; }
	osg::Vec3 getTarget() { return target; }
	
	void launch(osg::Vec3 direction, float force);
	bool update();
	void detonate();
	double getDetonateTime() { return mDetonateTime; }
	
	bool isArmed() { return armed; }
	
	void setYield(float yield) { mYield = yield; }
	float getYield() { return mYield; }
	
	void setOwner(Unit* unit) { mOwner = unit; }
	Unit *getOwner() { return mOwner.get(); }
		
	static std::set<dtCore::RefPtr<Projectile> > projectiles;
	
	static const std::string SHELL_TEXT;
	static const std::string BULLET_TEXT;
	
	ProjectileType getType() { return mType; }

	bool isDirectHit(Unit* u) { return mHit==u; }

private:
	float radius;
	float mass;
	dMass m;
	bool armed;
	osg::Vec3 target;
	float mYield;
	double mDetonateTime;
	ProjectileType mType;
	dtCore::RefPtr<Unit> mOwner;
	osg::Vec3 mPrevPosition;
	Unit* mHit;
	
	dtCore::RefPtr<dtCore::ParticleSystem> mExplosion;
};

#endif /*PROJECTILE_H_*/
