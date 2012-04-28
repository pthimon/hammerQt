#ifndef UNIT_H_
#define UNIT_H_

#pragma GCC diagnostic ignored "-Wextra"

#include "../common.h"

#include <dtCore/shaderprogram.h>
#include <dtCore/shaderparamfloat.h>
#include <dtCore/object.h>
#include <dtUtil/log.h>
#include <osg/Vec3>
#include <dtGUI/dtgui.h>

#include <vector>
#include <map>
#include <algorithm>

#include <boost/archive/xml_oarchive.hpp>
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/utility.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/nvp.hpp>

#pragma GCC diagnostic warning "-Wextra"

#include "../HTAppBase.h"
//#include "Projectile.h"
#include "UnitConfigData.h"
#include "PathInterpolation.h"

class UnitTank;
class UnitSoldier;
class Projectile;
class Observations;

class Unit : public dtCore::Object
{
public:
	Unit(const std::string& name = "UnknownUnit", osg::Vec3 position = osg::Vec3(0,0,0),
		 double facing = 0, UnitSide side = BLUE,
		 bool controlled = true, bool remote = false, bool flying = false, int client = -1, int id = -1);
	virtual ~Unit();
	Unit(Unit&);
	virtual Unit* clone();

	//Serialisation
	friend class boost::serialization::access;
	/*template<class Archive>
	void serialize(Archive & ar, const unsigned int version) {
		ar & m_Health;
	}*/
	template<class Archive>
	void save(Archive & ar, const unsigned int version) const
    {
		//unit constructor properties
        ar << boost::serialization::make_nvp("m_Name",GetName());
        ar << BOOST_SERIALIZATION_NVP(mUnitID);
        ar << BOOST_SERIALIZATION_NVP(mClientID);

        osg::Vec3 pos;
        double h;
        if (theApp->isLocalInstance(mClientID)) {
        	pos = const_cast<Unit*>(this)->getPosition();
        	h = const_cast<Unit*>(this)->getHeading();
        } else {
        	pos = m_StartPosition;
        	h = m_StartHeading;
        }
        ar << boost::serialization::make_nvp("m_Position_x", pos[0])
        	<< boost::serialization::make_nvp("m_Position_y", pos[1])
        	<< boost::serialization::make_nvp("m_Position_z", pos[2]);
        ar << boost::serialization::make_nvp("m_Heading", h);
        ar << BOOST_SERIALIZATION_NVP(m_Side);
        ar << BOOST_SERIALIZATION_NVP(m_IsControlled);
        ar << BOOST_SERIALIZATION_NVP(m_IsRemote);
        ar << BOOST_SERIALIZATION_NVP(m_IsFlying);

        //variables
        ar << BOOST_SERIALIZATION_NVP(m_Health);
    }
	template<class Archive>
	void load(Archive & ar, const unsigned int version)
    {
		//unit constructor properties
		std::string name;
		ar >> boost::serialization::make_nvp("m_Name",name);
        SetName(name); //dtCore::Object
        setName(name); //OSG node
        ar >> BOOST_SERIALIZATION_NVP(mUnitID);
        ar >> BOOST_SERIALIZATION_NVP(mClientID);
        float x,y,z;
        ar >> boost::serialization::make_nvp("m_Position_x", x)
    		>> boost::serialization::make_nvp("m_Position_y", y)
    		>> boost::serialization::make_nvp("m_Position_z", z);
        m_StartPosition = osg::Vec3(x,y,z);
        ar >> boost::serialization::make_nvp("m_Heading", m_StartHeading);
        ar >> BOOST_SERIALIZATION_NVP(m_Side);
        ar >> BOOST_SERIALIZATION_NVP(m_IsControlled);
        ar >> BOOST_SERIALIZATION_NVP(m_IsRemote);
        ar >> BOOST_SERIALIZATION_NVP(m_IsFlying);

        //variables
        ar >> BOOST_SERIALIZATION_NVP(m_Health);

        //add unit to scene and initialize physics
        if (theApp->isLocalInstance(mClientID) && !theApp->isAddingPrediction()) {
        	theApp->AddUnit(this);
        }
        if (theApp->isAddingPrediction()) {
        	mPredictedUnit = true;
        }
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()

	/** second phase of two-stage initialization
	 * (thats because transformable.cpp contains a PrePhysicsStepUpdate() step
	 * when SetTranform() is used)
	 */
	virtual void init();

	//parse XML configuration file
	bool parseUnitConfigFile(const std::string& file);
	virtual bool applyConfigData(const UnitConfigData& data);
	bool applyUnitConfigData(const UnitConfigData& data);

	// update the unit
	virtual void update();

	// returns the map which stores which children are
	// broadcasted over the network and what are their mesh file
	const std::map<dtCore::Transformable*, std::string>& GetNetworkingChildrenMeshes() const
		{	return m_NetworkingChildrenMeshes; }

	virtual osg::Vec3 getPosition();
	virtual bool setPosition(osg::Vec3 pos);
	virtual osg::Vec3 validatePosition(osg::Vec3 pos);

	osg::Vec3 getVelocity();

	std::string getName() const { return GetOSGNode()->getName(); }
	void setName(std::string name) { GetOSGNode()->setName(name); SetName(name); }

	virtual double getHeading();
	virtual osg::Vec3 getHeadingVector();
	virtual bool setHeading(double facing);
	virtual osg::Quat getQuat();
	virtual osg::Vec3 getNormal();

	UnitSide getSide() const { return m_Side; }
	bool setSide(UnitSide side) { m_Side = side; setSelected( isSelected() ); return true; }

	bool getIsFlying() const { return m_IsFlying; }
	bool setIsFlying(bool flying) { m_IsFlying = flying; return true; }

	bool getIsControlled() const { return m_IsControlled; }
	bool setIsControlled(bool controlled) { m_IsControlled = controlled; return true; }

	bool getIsRemote() const { return m_IsRemote; }
	bool setIsRemote(bool remote) { m_IsRemote = remote; return true; }

	osg::Vec3 getGoalPosition();
	bool addGoalPosition(osg::Vec3 goal);
	virtual bool setGoalPosition(osg::Vec3 goal, bool marker=true);
	
	/// sets the path as the goal path
	bool setGoalPath(PathInterpolation path, bool marker=true);

	osg::Vec3 getFinalGoalPosition();

	float getAngleToGoal();
	float getDistanceToGoal();

	MarkerType getMarker() const { return m_Marker; }
	void setMarker(MarkerType marker) { m_Marker = marker; }

	virtual void applyShader();
	void updateShaderParams();

	void setSelected(bool sel);
	bool isSelected();

	float getHealth() const { return m_Health; }
	float decrementHealth(float by);
	void setHealth(float h) { m_Health = h; }

	void setHealthWindow(CEGUI::Window *w) { mHealthWindow = w; }

	void addUnitMarker();
	void removeGoalMarker();

	float getHeadingToPosition(osg::Vec3 goalPos);
	float getPitchToPosition(osg::Vec3 goalPos);

	bool isEnemy(Unit *unit) const;

	CEGUI::Window *getHealthWindow() { return mHealthWindow; }

	double getArmour() { return mArmour; }
	std::string getType() { return m_UnitType; }

        float getSensorRangeIn(std::string id) { return m_SensorRange[id]; }
        float getVisibilityIn(std::string id) { return m_Visibility[id]; }

	void setUnitMarkerAlpha(float alpha);

	//the client that this unit is being simulated on
	void setClientID(int id) { mClientID = id; }
	int getClientID() const { return mClientID; }

	//this id is easier to work with than the name string
	void setUnitID(int id) { mUnitID = id; }
	int getUnitID() const { return mUnitID; }

	/**
	 * After scheduled observations are made, this property is updated with
	 * the last observed position of the unit
	 */
	osg::Vec3 getObservedPosition();
	bool isObserved();
	bool isVisible();
	virtual void setVisible(bool visibility);
protected:
	osg::Vec3 setObservedPosition(osg::Vec3 centre, float radius, float resolution);
public:
	virtual double getObservedPositionVariance();
	osg::BoundingBox getObservedBoundingBox();
	osg::BoundingBox getMaxBoundingBox();

	//void setGroup(UnitGroup* group);
	//UnitGroup* getGroup();

	virtual float getMaxSpeed() { return 1; }
	virtual float getSpeed(float x, float y) { return 1; }

protected:
	/** Load all the meshes and add them to the scene
	 * default: no meshes
	 * It should do 3 things:
	 * 1. load the meshes,
	 * 2. move them into proper relative position,
	 * 3. register the child objects (via the AddChild() method),
	 * 4. register the child objects for which current transformation should
	 * 	  be broadcasted over the network (via the RegisterNetworkingChild() method)
	 */
	virtual void LoadMeshes() {}
	/// default: no physics
	virtual void initPhysics() {}
	/// default: no physics
	virtual void updatePhysics() {}

	void RegisterNetworkingChild(dtCore::Transformable* child, std::string mesh_file)
	{
		m_NetworkingChildrenMeshes[child] = mesh_file;
	}

	void UnRegisterNetworkingChild(dtCore::Transformable* child)
	{
		m_NetworkingChildrenMeshes.erase(child);
	}

	void addWaypointMarker(osg::Vec3 startPos, osg::Vec3 endPos);
	dtCore::Object* createWaypointMarker(osg::Vec3 startPos, osg::Vec3 endPos);
	void updateWaypointMarker();
	void updateUnitMarker();

	// which side does it belong to?
	UnitSide m_Side;
	// can the unit fly?
	bool m_IsFlying;
	// can the unit be controlled by the user?
	bool m_IsControlled;
	// is the unit updated remotely?
	bool m_IsRemote;
	// is the unit already initialized by init()?
	bool m_Initialized;
	// the type of unit (tank|soldier)
	std::string m_UnitType;
	// sensor range while in a terrain type of a given layer id
	std::map<std::string, float> m_SensorRange;
        // visibility in a terrain type of a given layer id
        std::map<std::string, float> m_Visibility;

	double m_StartHeading;
	float m_Health;

	osg::Vec3 m_StartPosition;
	osg::Vec3 m_GoalPosition;

	//shader stuff
	std::string mShaderEffect;

	float mShaderTextureCycleTime;
	float mShaderMoveXCycleTime;
	float mShaderMoveYCycleTime;
	float mShaderMoveZCycleTime;

	dtCore::RefPtr<dtCore::ShaderProgram> mCurrentShader;
	dtCore::RefPtr<dtCore::ShaderParamFloat> mTextureDilationParam;
	dtCore::RefPtr<dtCore::ShaderParamFloat> mMoveXDilationParam;
	dtCore::RefPtr<dtCore::ShaderParamFloat> mMoveYDilationParam;
	dtCore::RefPtr<dtCore::ShaderParamFloat> mMoveZDilationParam;
	std::string mCurrentShaderName;

	// gui stuff
	CEGUI::Window *mHealthWindow;

	// unit marker on the minimap
	MarkerType m_Marker;

	//goal marker
	// dtCore::Object* mGoalMarker;
	//dtCore::RefPtr<dtCore::Object> mGoalMarker;
	//dtCore::RefPtr<osg::Geode> mGoalMarkerGeode;
	std::vector<dtCore::RefPtr<dtCore::Object> > mWaypointMarkers;

	dtCore::RefPtr<dtCore::Object> mUnitMarker;
	dtCore::RefPtr<osg::Geode> mUnitMarkerGeode;

	// networking
	// a vector storing the mesh files to initialize with
	std::map<dtCore::Transformable*, std::string> m_NetworkingChildrenMeshes;

	osg::Vec3 mLastPosition;
	osg::Vec3 mLastPositionTemp;
	double mLastPositionTime;
	double mLastPositionTimeTemp;
	double mArmour;
	double mUnitMarkerAlpha;
	bool mSelected;

	dtCore::RefPtr<PathInterpolation> mPath;
	unsigned int mNextWaypoint;

	int mClientID;
	int mUnitID;

	unsigned long int mForestMask, mRoadMask;

	bool mPredictedUnit;

	//allow the observations class to set the observed position
	friend class Observations;
	osg::Vec3 mObservedPosition;
	double mObservedPositionVariance;
	double mObservedPositionTime;
	float mObMaxDistance;
	float mObMaxVariance;

	bool mVisible;
};

#endif /*UNIT_H_*/
