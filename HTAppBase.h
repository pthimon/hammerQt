#ifndef HTAPPBASE_H_
#define HTAPPBASE_H_

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wextra"

#include <osg/ApplicationUsage>
#include <osg/ArgumentParser>

#include <dtCore/refptr.h>
#include <dtCore/globals.h>
#include <dtCore/infinitelight.h>
#include <dtCore/object.h>

#include <dtGame/logstatus.h>
#include <dtGame/logtag.h>
#include <dtGame/logkeyframe.h>
#include <dtGame/loggermessages.h>
#include <dtGame/message.h>
#include <dtGame/defaultmessageprocessor.h>

#include <dtTerrain/terrain.h>
#include <dtTerrain/soarxterrainrenderer.h>
#include <dtTerrain/geocoordinates.h>
#include <dtTerrain/terraindecorationlayer.h>
#include <dtTerrain/vegetationdecorator.h>
#include <dtTerrain/lccanalyzer.h>
#include <dtTerrain/lcctype.h>
#include <dtTerrain/geotiffdecorator.h>
#include <dtTerrain/colormapdecorator.h>

#include <dtABC/weather.h>

#include <dtCore/environment.h>
#include <dtCore/scene.h>

#include <dtUtil/exception.h>

#include <osg/Version>
#include <osg/BoundingBox>
#include <sstream>
#include <fstream>

// include headers that implement a archive in simple text format
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/xml_oarchive.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/serialization/set.hpp>
#include <boost/serialization/nvp.hpp>

#include <gsl/gsl_rng.h>

#include "common.h"
//#include "mynetwork.h"
#include "gui/mapmotionmodel.h"
#include "gui/guihud.h"
//#include "gui/pick.h"
//#include "UnitTank.h"
//#include "Unit.h"
//#include "Projectile.h"
#include "terrain/terrain.h"
//#include "baseabc_HT.h"
#include "event.h"
//#include "ScenarioData.h"
#include "models/inc/TerrainMap.h"
//#include "models/InverseModel.h"
#include "logic/PlanAdaptor.h"
#include "logic/PlanCombiner.h"
#include "models/games/Game.h"

#pragma GCC diagnostic warning "-Wextra"

class Unit;
class UnitFiring;
class MyNetwork;
class UnitInitPacket;
class UnitSavePacket;
class Projectile;
class ScenarioData;
class InverseModel;
class Targets;
class UnitGroup;
class UnitObserver;
class Observations;
class Watchdog;
class HypothesisHost;

namespace dtGame
{
   class GameManager;
   class GameActorProxy;
   class ServerLoggerComponent;
   class LogController;
   class TaskComponent;
}

struct ReplayCommand {
	ReplayCommand(double t, Unit* u, osg::Vec3 p):time(t),unit(u),pos(p) { }
	double time;
	Unit* unit;
	osg::Vec3 pos;
};

class HTAppBase : public dtABC::BaseABC
{
public:
	HTAppBase( const std::string& configFilename,
		const std::string& hostName, int port, bool wait_for_client, const std::string& scenarioFilename, int id, std::string& replayFile, std::string& configFile);

	virtual void Init();

	//Override from baseABC to create a scene with our own physics controller
	virtual void CreateInstances();

	virtual void SetupNetwork();
	virtual void CreateEnvironment();
        virtual void CreateLogic();
	virtual void Config();
	virtual void CreateActors();
	virtual void InitHypothesisManager();

    ///Start the Application
    virtual void Run();

    //Restart the scenario
    virtual void restartScenario();

	/**
     * Override of dtABC::PreFrame
     * Called automatically before each frame
     */
    virtual void PreFrame( const double deltaFrameTime );
    virtual void Frame( const double deltaFrameTime ) {};
    virtual void PostFrame( const double deltaFrameTime ) {};

    /**
     * PreFrame function for subclasses to override
     * Ensures that the event log interval is still valid
     */
    virtual void PreFrameGUI( const double deltaFrameTime );

    Unit *getUnitByName(std::string name);

    //terrain functions
    /** Check whether a given coordinate is within map boundaries and above the terrain.
     * @return True if yes, false if the position is out of boundaries.
     */
     bool IsValidPosition(osg::Vec3 pos);

    /** Check whether a given coordinate is within map boundaries.
     * @return True if yes, false if the position is out of boundaries.
     */
     bool IsValidXYPosition(float x, float y);

    /** Gets a random valid position. Height is set to the actual terrain height.
     * @returns A random position.
     */
	 osg::Vec3 GetRandomPosition();

    /** Gets a random valid position based on predefined terrain size. Height is always 0.
     * @returns A random position.
     */
	 osg::Vec3 PreGetRandomPosition();

    /**
     * Returns the terrain height at a given x and y coordinate.
     * @params h Returns the height.
     * @params Return false if there is no terrain below the given position, true otherwise.
     */
    bool GetTerrainHeight(double x, double y, double & h);

    /**
     * Returns nearest point on terrain to p
     */
    osg::Vec3 mapPointToTerrain(osg::Vec3 p);

    /**
     * ODE Collision callback
     * Called automatically by ODE when objects are near
     */
    static void nearCallback (void *data, dGeomID o1, dGeomID o2);

    /** Finish the game with the given winner.
     */
    virtual void EndGame( UnitSide winner );
    virtual void EndGameGUI();

    std::string getTerrainName() { return m_sWorldName; }
	Terrain *getTerrain() { return m_Terrain.get(); }
        PlanAdaptor *getPlanAdaptor() { return m_PlanAdaptor.get(); }
        PlanCombiner *getPlanCombiner() { return m_PlanCombiner.get(); }

	double getSimTime() { return m_SimTime; }
	double getFrameTime() { return m_FrameTime; }

	osg::Vec2 CalcMapPosFromWorldPos(osg::Vec2 world_pos) const;
	osg::Vec2 CalcWorldPosFromMapPos(osg::Vec2 map_pos) const;

	float getGravity() { return m_Gravity; }

	/// returns a list of units visible from the current position of the given unit
	std::set<Unit*> SearchVisibleTargets(UnitFiring & from_unit);

	virtual void Quit();

	virtual void CreateRemoteUnit(UnitInitPacket& packet);
	virtual void GetPositionOfNetObject(std::string ID, osg::Vec3& XYZ, osg::Vec3& HPR);
	virtual void SendPosition(std::string ID, osg::Vec3& XYZ, osg::Vec3& HPR);

	virtual void BroadcastEvent(Event::EventType e, Unit *unit1 = NULL, Unit *unit2 = NULL, osg::Vec3 pos = osg::Vec3(NAN,NAN,NAN));
	virtual void ReceiveEvent(Event::EventType event, std::vector<std::string>& participants, std::vector<double>& data);

	UnitSide getSide() { return m_MySide; }

	Unit* PointToUnit(osg::Vec3 point);
	Unit* TrajectoryToUnit(osg::Vec3 start, osg::Vec3 end);

	///  get the set of all units
	/// (if possible, avoid using it, needed by UnitFiring to refresh the old target list)
	std::set<Unit*> GetAllUnits();

	// focuses on the unit with the given name
	virtual void FocusOnUnit(const std::string & name) {};

	virtual void displayUnitHealth(Unit *unit, float health) {};
	virtual void displayUnitReload(UnitFiring *unit, double reload) {};

	virtual void zoomUpdate(float dist) {};

	virtual void setSelectedFormation(std::string formation) {};

	bool isPathPlanningEnabled() { return mPathPlanning; }
	void togglePathPlanning();
	TerrainMap *getTerrainMap() { return mTerrainMap; }

	virtual void loadFile(std::string filename);
	virtual void saveFile(std::string filename);

	int getInstanceID() {
		return m_InstanceID;
	}

	virtual void clearScene();
	void sendUnit(Unit *unit, int total = 1, int client = -1);
	void sendLocalUnits(int client = -1);
	bool isLocalInstance(int id);
	//is this client a forward model?
	bool isForwardModel();

	//sends the current world state to a client over the network
	void sendWorldState(int client);
	//reads in the world state from unit save packets
	void loadWorldState(std::vector<std::string> *units);
	Unit *deserialiseUnit(std::string unitStr, bool prediction = false);

	int getControllerId() { return m_ControllerId; }
	bool hasController();

	bool getIsServer();

	virtual bool isHeadless();

	bool unitExists(Unit* unit);
	Unit* getUnitById(int id);
	std::vector<Unit*> getUnitsByIds(std::vector<int>& ids);

	void addInverseModel(InverseModel *model);

	void setTimeScale(double timescale);
	void multiplyTimeScale(double timescale);
	double getTimeScale();

	MyNetwork* getNetwork() { return m_Net.get(); }
	int getNetworkPort() { return m_PortNum; }

	bool isAddingPrediction();

	Targets* getTargets() { return mTargets; }

	double getRandomGaussian(double sigma);

	std::string getStartTimeString();

	int loadReplay(std::string replayFile);
	bool isReplay() { return mReplay; }

	void newTriggered();

	void setUnitName(int id, std::string name);
	std::string getUnitName(int id);

	UnitObserver* createObserver(UnitObserverType type);
	void triggerLaunchHypotheses();
	void saveAll();
	void launchHypotheses();
	std::string getHostname();

	float getObservationDuration();
	float getSpeedup();

	void setSides(UnitSide attackingSide, UnitSide defendingSide) {
		mAttackingSide = attackingSide;
		mDefendingSide = defendingSide;
	}

protected:
    virtual ~HTAppBase();
    virtual bool parseScenarioFile(const std::string& file);
public:
    virtual void AddUnit(Unit* unit);
protected:
    virtual void RemoveUnit(Unit* unit, bool clear = false);

    Observations* mObservations;

    bool mLaunch;

	//stores units added to the scene to loop through them in preframe
	std::set< dtCore::RefPtr<Unit> > m_Units;
	typedef std::set< dtCore::RefPtr<Unit> >::iterator unit_it;

	// Environment
	dtCore::RefPtr<dtCore::Environment> m_Environment;
	dtCore::RefPtr<dtCore::InfiniteLight> mInfiniteLight;

   dtCore::RefPtr<Terrain> m_Terrain;
   dtCore::RefPtr<PlanAdaptor> m_PlanAdaptor;
   dtCore::RefPtr<PlanCombiner> m_PlanCombiner;

   std::string m_sWorldName;

   //ODE collision contact group
   dJointGroupID m_ContactGroup;

   //std::vector< std::pair<MarkerType,Unit*> > m_UnitMarkers;

   float m_Gravity;

   dtCore::RefPtr<MyNetwork> m_Net; ///<Reference the NetMgr derived class

   std::string m_HostName; ///<The hostname to connect to (if we're a client)
   int m_PortNum;
   std::string m_MapName;
   bool m_WaitForClient;

   double m_SimTime;
   double m_FrameTime;

   // this variable stores this client's side
   UnitSide m_MySide;

   // paused by
   bool m_RedPaused;
   bool m_BluePaused;

   // id
   int m_InstanceID;

   // number of units local to this network instance
   int m_NumRed;
   int m_NumBlue;

   std::string mScenarioFilename;
   ScenarioData *mScenarioData;

   // the instance id of the controller (none: -1)
   int m_ControllerId;

   TerrainMap *mTerrainMap;

   bool mPathPlanning;

   std::vector<dtCore::RefPtr<InverseModel> > mInverseModels;

   //set by the controller when it resends the scenario data in the init packet to its clients
   bool m_ForwardModel;

   // the next one stores that for how long are red units on the goal area
   double m_AttackerOnGoalTime;

   // marks whether we enable pausing
   bool m_PauseEnabled;

   // dont start counting whether all the opponents units are dead until they're all received
   int m_NumRemoteUnits;
   int m_NumRemoteUnitsReceived;

   double mRemainingTime;

   bool mAddingPrediction;

   Targets* mTargets;

   std::vector<dtCore::RefPtr<UnitGroup> > mGroups;

   gsl_rng *mRng;

   std::string mStartTimeStr;

   std::string mReplayFile;

   std::vector<ReplayCommand*> mReplayCommands;
   unsigned int mReplayCommandIndex;
   bool mReplay;

   bool mReset;

   std::map<int, std::string> mUnitNames;

   std::string mConfigFile;
   unsigned int mNumClients;
   bool mLaunchClients;
   std::vector<HypothesisHost*> mClientHosts;

   Game* mGame;

   Watchdog* mWatchdog;

   UnitSide mAttackingSide;
   UnitSide mDefendingSide;
};

#endif /*HTAPPBASE_H_*/
