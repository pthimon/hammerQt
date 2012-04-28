#ifndef UNITFIRING_H_
#define UNITFIRING_H_

#include "Unit.h"
#include "Projectile.h"

class UnitFiring : public Unit
{
public:
	UnitFiring(const std::string& name = "UnknownFiringUnit", osg::Vec3 position = osg::Vec3(0,0,0),
		 double facing = 0, UnitSide side = BLUE,
		 bool controlled = true, bool remote = false, bool flying = false, int client = -1, int id = -1 );
	//UnitFiring(UnitFiring& u);
	virtual ~UnitFiring();
	virtual UnitFiring* clone();

	//Serialisation
	friend class boost::serialization::access;
	template<class Archive>
	void save(Archive & ar, const unsigned int version) const
    {
		ar << BOOST_SERIALIZATION_BASE_OBJECT_NVP(Unit);

		double reloadTime = getRemainingReloadTime();
		ar << boost::serialization::make_nvp("m_ReloadTime", reloadTime);
		//TODO save m_VisibleUnits set? Will generate extraneous VIS/HIDE logs.
    }
	template<class Archive>
	void load(Archive & ar, const unsigned int version)
    {
		ar >> BOOST_SERIALIZATION_BASE_OBJECT_NVP(Unit);

		double reloadTime;
		ar >> boost::serialization::make_nvp("m_ReloadTime", reloadTime);
		m_FiredTime = theApp->getSimTime() - (m_ReloadTime - reloadTime);
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()

	void update();

	void autoFire();
	virtual bool fire();
	bool launch(osg::Vec3 from, osg::Vec3 target);
        // returns false if target is too far
        bool getFiringAngle(osg::Vec3 target, float & angle);

	//time to reload, in seconds
	double getReloadTime() { return m_ReloadTime; }
	void setReloadTime(double r) { m_ReloadTime = r; }
	double getRemainingReloadTime() const;

	bool isLoaded();
	void reload();

	//stores projectile range squared (for efficient comparision with vector.length2)
	void setProjectileRange(float range) { mProjectileRange2 = range * range; }
	float getProjectileRange() { return sqrt(mProjectileRange2); }
	void setProjectileRange2(float range2) { mProjectileRange2 = range2; }
	float getProjectileRange2() { return mProjectileRange2; }

	bool isTargetInRange(osg::Vec3 target);

	void setTargetPosition(osg::Vec3 t) { mTargetPosition = t; mAimReset = false; }

	void setReloadWindow(CEGUI::Window *w) { mReloadWindow = w; }

	/**
	 * Finds the nearest tank that is in line of sight, stored in mTargetPosition
	 * @returns whether target has been found
	 */
	bool selectTarget();

	void calcAimParams();
	virtual void aim();
	virtual bool isAimed();
	virtual void freezeAim() { }
	virtual void resetAim() { }

	osg::Vec3 calcFireDirection(float headingAngle, float pitchAngle);

        bool getFiringPitchToPosition(osg::Vec3 target, float & angle);

	osg::Vec3 predictTargetPosition(Unit *unit);

	virtual osg::Vec3 getProjectileLaunchPos();

	CEGUI::Window *getReloadWindow() { return mReloadWindow; }

	void setAutoFire(bool enabled) {
		mAutoFire = enabled;
	}

protected:

	virtual bool applyConfigData(const UnitConfigData& data);
	bool applyUnitFiringConfigData(const UnitConfigData& data);

	double m_FiredTime;
	osg::Vec3 mTargetPosition;
	float mAimHeading;
	float mAimPitch;
	bool mAimReset;
	bool mTargetAquired;
	const Unit *mTargetUnit;
	std::set<Unit*> mVisibleUnits;
	std::map<Unit*, double> mSeesUnitsTime;
	std::map<Unit*, double> mHideUnitsTime;

	// projectile stuff
	double m_ReloadTime;
	double mProjectileMass;
	double mProjectileRadius;
	double mProjectileVelocity;
	float mProjectileYield;
	float mProjectileRange2;
	Projectile::ProjectileType mProjectileType;

	CEGUI::Window *mReloadWindow;

	bool mAutoFire;
};

#endif /*UNITFIRING_H_*/
