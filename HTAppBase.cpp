#include "common.h"

#include "HTAppBase.h"
#include "mynetwork.h"
#include "packets.h"
#include "eventlog.h"
#include "ScenarioData.h"
#include "ScenarioHandler.h"
#include "units/UnitTank.h"
#include "units/UnitRemote.h"
#include "units/UnitSoldier.h"
#include "units/UnitGroup.h"
#include "units/UnitObserver.h"
#include "models/inc/InverseModel.h"
#include "HTPhysicsController.h"
#include "models/inc/Targets.h"
#include "models/inc/Observations.h"
#include "hypotheses/HypothesisManager.h"
#include "hypotheses/HypothesisClient.h"
#include "models/games/DefaultGame.h"
#include "models/games/EvasionGame.h"
#include "models/games/DirectionGame.h"
#include "models/games/CaptureBaseGame.h"
#include "models/games/MazeGame.h"
#include "Watchdog.h"

#pragma GCC diagnostic ignored "-Wextra"

#include <dtCore/transform.h>
#include <dtCore/globals.h>
#include <dtCore/object.h>
#include <dtCore/orbitmotionmodel.h>
#include <dtCore/camera.h>
#include <dtCore/scene.h>
#include <dtCore/object.h>
#include <dtCore/enveffect.h>
#include <dtCore/deltawin.h>
#include <dtCore/infinitelight.h>
#include <dtCore/object.h>
#include <dtCore/globals.h>
#include <dtCore/orbitmotionmodel.h>
#include <dtCore/camera.h>
#include <dtCore/system.h>
#include <dtCore/skydome.h>
#include <dtCore/infiniteterrain.h>
#include <dtCore/isector.h>
#if DELTA3D_VER >= 23
#include <ode/odeinit.h>
#endif

#include <dtCore/shadermanager.h>
#include <dtCore/shaderparameter.h>
#include <dtABC/application.h>
#include <dtUtil/mathdefines.h>
#include <dtUtil/boundingshapeutils.h>
#include <dtNet/dtnet.h>
#include <osg/Math>
#include <osg/LineSegment>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>
#include <ode/ode.h>
#include <cassert>
#include <queue>
#include <sstream>
#include <cstdlib>
#include <map>
#include <fstream>
#include <boost/lexical_cast.hpp>

#include <gnelib/GNEDebug.h>

#include <dtUtil/xercesparser.h>
#include <dtUtil/xercesutils.h>
#include <xercesc/util/XMLString.hpp>

#include <gsl/gsl_randist.h>

#include <QDateTime>
#include <QtXml/QtXml>
#include <unistd.h>

#pragma GCC diagnostic warning "-Wignored-qualifiers"
#pragma GCC diagnostic warning "-Wextra"

const float cMapHorizontalSize = 5000.0f;

HTAppBase::HTAppBase( const std::string& configFilename,
		const std::string& hostName, int port, bool wait_for_client, const std::string& scenarioFilename, int id, std::string& replayFile, std::string& configFile) :
	dtABC::BaseABC("Application"),
	mLaunch(false),
	m_HostName(hostName),
	m_PortNum(port),
	m_WaitForClient(wait_for_client),
	m_SimTime(0),
	m_MySide(RED),
	m_RedPaused(false),
	m_BluePaused(false),
	m_InstanceID(id),
	mScenarioFilename(scenarioFilename),
	mPathPlanning(false),
	m_ForwardModel(false),
	m_AttackerOnGoalTime(0),
        m_PauseEnabled(true),
	m_NumRemoteUnits(0),
	m_NumRemoteUnitsReceived(0),
	mAddingPrediction(false),
	mReplayFile(replayFile),
	mReplayCommandIndex(0),
	mReplay(false),
	mConfigFile(configFile),
	mNumClients(2),
	mLaunchClients(true),
	mGame(NULL),
	mAttackingSide(RED),
	mDefendingSide(BLUE)
{
	dtUtil::LogFile::SetFileName("delta3d_log_"+boost::lexical_cast<std::string>(id)+".html");
	mRng = gsl_rng_alloc (gsl_rng_default);
	QDateTime date = QDateTime::currentDateTime();
	mStartTimeStr = date.toString("yyyyMMdd-hh:mm:ss").toStdString();

	CreateInstances();

	// initialize shaders
	dtCore::ShaderManager::GetInstance().LoadShaderDefinitions("Shaders/ShaderDefs.xml", false);

	mClientHosts.push_back(new HypothesisHost("localhost",1,"~/development/workspace/hammerQt", "hammerQt"));

	if (m_HostName.empty()) {
		//start a 10 second timeout on the main loop (on server)
		mWatchdog = new Watchdog(10);
		mWatchdog->go();
	}
}

HTAppBase::~HTAppBase()
{
	/*for (unsigned int i=0; i<m_Units.size(); i++)
		delete m_Units[i];*/
	/*for (unsigned int i=0; i<Projectile::projectiles.size(); i++)
		delete Projectile::projectiles[i];
	for (unsigned int i=0; i<Projectile::projectilesToDetonate.size(); i++)
		delete Projectile::projectilesToDetonate[i];*/
	for (unsigned int i=0; i<mClientHosts.size(); i++)
		delete mClientHosts[i];

	delete mWatchdog;

    dJointGroupDestroy(m_ContactGroup);
}

void HTAppBase::Init() {
#if DELTA3D_VER >= 23
        dInitODE2(0);
#endif
    //instantiate observation manager
    mObservations = new Observations();
	SetupNetwork();
	CreateEnvironment();
	CreateLogic();
	Config();
	CreateActors();
	int replay = loadReplay(mReplayFile);
	if (replay < 0) {
		LOG_ERROR("Could not load replay file")
	} else {
		mReplay = true;
	}
	InitHypothesisManager();
}

///Override baseABC to create the basic instances, and call Scene with our own physics controller
void HTAppBase::CreateInstances() {
	// create the camera
   assert(mViewList[0].get());

   mViewList[0]->SetCamera(new dtCore::Camera("defaultCam"));

   HTPhysicsController* physics = new HTPhysicsController(this);
   physics->SetMaxPhysicsStep(0.2);
   mViewList[0]->SetScene(new dtCore::Scene(physics, "defaultScene"));

   GetKeyboard()->SetName("defaultKeyboard");
   GetMouse()->SetName("defaultMouse");
}

void HTAppBase::SetupNetwork() {
	m_Net = new MyNetwork( GetScene() );

	std::string logFilename;

	if (m_HostName.empty()) logFilename = std::string("server.log");
	else logFilename = std::string("client.log");

	//initialize the game name, version number, and a networking log file
	m_Net->InitializeGame("HTNetwork", 1, logFilename);

#ifdef DEBUG
	GNE::initDebug(GNE::DLEVEL1 | GNE::DLEVEL2 | GNE::DLEVEL3 | GNE::DLEVEL4 | GNE::DLEVEL5, logFilename.c_str());
#endif

	//register our custom packet with GNE
	//must come *after* NetMgr::InitializeGame()
	GNE::PacketParser::defaultRegisterPacket<PositionPacket>();
	GNE::PacketParser::defaultRegisterPacket<PlayerQuitPacket>();
	GNE::PacketParser::defaultRegisterPacket<InitializingPacket>();
	GNE::PacketParser::defaultRegisterPacket<InitializingUnitsPacket>();
	GNE::PacketParser::defaultRegisterPacket<UnitInitPacket>();
	GNE::PacketParser::defaultRegisterPacket<EventPacket>();
	GNE::PacketParser::defaultRegisterPacket<IdPacket>();
	GNE::PacketParser::defaultRegisterPacket<UnitSavePacketRequest>();
	GNE::PacketParser::defaultRegisterPacket<UnitSavePacket>();
	GNE::PacketParser::defaultRegisterPacket<CommandPacket>();

	//--- setup network connection and mScenarioData ---
	// if no hostname was supplied, create a server, otherwise create a client

	if (m_HostName.empty()) // SERVER
	{
  	    LOG_INFO("Acting as server.")
        std::string scenarioPath = "";

		//get path from filename
		if (!mScenarioFilename.empty()) {
			scenarioPath = dtCore::FindFileInPathList( "scenarios/" + mScenarioFilename );
			if (scenarioPath.empty()) {
			     LOG_ERROR("Could not find scenario file to load.  Missing file name is """ + mScenarioFilename + """" );
			}
		}

        //parse
        if (!parseScenarioFile(scenarioPath)) {
        	LOG_ERROR("Could not parse scenario XML file: "+ mScenarioFilename);
        }

        //server is always blue (used for showing paused messages) TODO replace this with m_InstanceID
        m_MySide = BLUE;

		// assemble and set initializing packet for client(s)
		InitializingPacket mypacket(mScenarioData);
		m_Net->SetInitializingPacket(mypacket);

		// now wait for client to connect if told so

		if (mScenarioData->numInstances > 1) {
			m_Net->SetupServer( m_PortNum );
			for (int i=0; i < mScenarioData->numInstances; i++) {
				std::ostringstream oss;
				oss << "Waiting for " << (mScenarioData->numInstances - (i+1)) << " clients to connect...";
				LOG_INFO(oss.str());

				// block until a client is connected TODO this only works for 1 client?
				m_Net->WaitForClient();
			}
		}
		if (m_WaitForClient) {
			LOG_INFO("Waiting for the controller to connect...");
			m_Net->SetupServer( m_PortNum );
			// block until a client is connected
			m_Net->WaitForClient();
		}


	}
	else // CLIENT
	{
		LOG_INFO("Acting as client.")
		m_Net->SetupClient( m_HostName, m_PortNum );

		// client is always red
		m_MySide = RED;

		// wait for initializing data from server
		LOG_INFO("Waiting for the server for initial data...");

		if (!m_Net->WaitForServer()) {
			LOG_ERROR("No data received from the server, giving up...");
			Quit();
		}

		InitializingPacket mypacket = m_Net->GetInitializingPacket();
		mScenarioData = mypacket.mScenarioData;

	}

	m_MapName = mScenarioData->mapName;
	for (unsigned int i=0; i < mScenarioData->sides.size(); i++) {
		int total = mScenarioData->sides[i]->total;
		switch (mScenarioData->sides[i]->side) {
		case BLUE:
			m_NumBlue = total;
			break;
		case RED:
		default:
			m_NumRed = total;
			break;
		}
	}
	m_ControllerId = mScenarioData->controllerId;
	m_ForwardModel = mScenarioData->forwardModel;
	mRemainingTime = mScenarioData->timeLimit;

    //--- setup logging ---
  	EventLog::SetLogSide(m_MySide);
  	//setup event logger for 1 second interval of logging of position and heading (after we know which side we are)
	EventLog::GetInstance().setLogInterval(1);
	//initialise logger with the number of units on each side
	EventLog::GetInstance().init(m_NumBlue, m_NumRed, m_MapName);

	if (!mConfigFile.empty()) {
		EventLog::GetInstance().logEvent("LOG  Using config file "+mConfigFile);
	}

	std::string game = mScenarioData->gameType;
	if (game.empty() || game == "default") {
		mGame = new DefaultGame();
	} else if (game == "evasion") {
		mGame = new EvasionGame();
	} else if (game == "direction") {
		mGame = new DirectionGame();
	} else if (game == "capture") {
		mGame = new CaptureBaseGame();
	} else if (game == "maze") {
		mGame = new MazeGame();
	}
}

bool HTAppBase::parseScenarioFile(const std::string& file)
{
   ScenarioHandler handler;
   dtUtil::XercesParser parser;
   bool parsed_well = parser.Parse(file, handler, "scenario.xsd");

   mScenarioData = handler.mScenarioData;

   return parsed_well;
}

void HTAppBase::CreateLogic()
  {
    m_PlanAdaptor = new PlanAdaptor();
    m_PlanCombiner = new PlanCombiner();
  }

   //////////////////////////////////////////////////////////////////////////
void HTAppBase::CreateEnvironment()
{
	//Create basic environment to attach terrain to
	dtABC::Weather *weather = new dtABC::Weather();
	weather->SetTheme(dtABC::Weather::THEME_FAIR);
  	m_Environment = weather->GetEnvironment();
  	m_Environment->SetDateTime(2005,6,21,20,0,0);
  	m_Environment->SetVisibility(1000000);
  	m_Environment->SetFogEnable(false);
  	m_Environment->SetFogMode(dtCore::Environment::EXP2);
  	// mEnvironment->SetSkyColor(osg::Vec3(0,0,0));

  	GetScene()->UseSceneLight(false);

  	mInfiniteLight = new dtCore::InfiniteLight(0,"MyLight");
#if DELTA3D_VER >= 23
  	dtCore::Transform transform;
  	transform.SetRotation(37.0f, 10.0f, 0.0f );
  	mInfiniteLight->SetTransform(transform);
#else
  	mInfiniteLight->SetAzimuthElevation( 37.0f, 10.0f );
#endif
  	mInfiniteLight->SetDiffuse( 255.0f, 255.0f, 255.0f, 1.0f );
  	mInfiniteLight->SetSpecular( 255.0f, 255.0f, 255.0f, 1.0f );
  	mInfiniteLight->SetEnabled( true );

  	GetScene()->AddDrawable( mInfiniteLight.get() );

  	m_Terrain = new Terrain(m_MapName, !isHeadless());

	//add the environment to the scene
	GetScene()->AddDrawable( m_Environment.get());

	//sync names
	m_sWorldName = m_Terrain->GetName();

	//create map (upon which to perform the A* search)
	mTerrainMap = new TerrainMap(m_Terrain.get());

	mTargets = new Targets(m_Terrain.get(), &m_Units, &mGroups);
}

void HTAppBase::Config()
{
	dtABC::BaseABC::Config();

	m_Gravity = -15.0f;
	GetScene()->SetGravity( 0.0f, 0.0f, m_Gravity );

	//Setup ODE collision callback
	m_ContactGroup = dJointGroupCreate (0);
	GetScene()->SetUserCollisionCallback(nearCallback, this);

	//Set physics updates to be a constant 60Hz (value needs tuning)
	//fixes large step size when resizing the screen
	//GetScene()->SetPhysicsStepSize(0.01);
	//dynamic_cast<Scene_HT*>(GetScene())->SetMaxPhysicsStep(0.02);
	GetScene()->SetPhysicsStepSize(0.02);
	//Max physics step moved to the create instances function
	//GetScene()->SetMaxPhysicsStep(0.16);

	dWorldSetAutoDisableFlag (GetScene()->GetWorldID(),0);
	dWorldSetERP ( GetScene()->GetWorldID() , 0.1f);
	dWorldSetCFM (GetScene()->GetWorldID(),1e-5);
	dWorldSetContactMaxCorrectingVel (GetScene()->GetWorldID(), 1.0f );
	//dWorldSetContactSurfaceLayer (GetScene()->GetWorldID(),0.001);
}

void HTAppBase::CreateActors()
{
	// create units instantiated on this network instance
	// all other units will be created when registered with the network
	int globalUnitIndex = 0;
	m_NumRemoteUnits = 0;
	for (unsigned int i=0; i < mScenarioData->sides.size(); i++) {
		int unitIndex = 0;
		for (unsigned int j=0; j < mScenarioData->sides.at(i)->units.size(); j++) {
			//create local units
			if (mScenarioData->sides.at(i)->units.at(j)->instance == m_InstanceID) {
				//
				for (int k=0; k < mScenarioData->sides.at(i)->units.at(j)->number; k++) {
					std::ostringstream oss;
					switch (mScenarioData->sides.at(i)->side) {
						case BLUE: oss << "Blue"; break;
						case RED: oss << "Red"; break;
						default: break;
					}
					osg::Vec2 pos2;
					if (mScenarioData->sides.at(i)->units.at(j)->number > 1) {
						//if number specified, linearly space them between a start and end position
						osg::Vec2 startPos = mScenarioData->sides.at(i)->units.at(j)->positions.at(0);
						osg::Vec2 endPos = mScenarioData->sides.at(i)->units.at(j)->positions.at(1);
						pos2 = ((endPos - startPos)/(mScenarioData->sides.at(i)->units.at(j)->number) * k) + startPos;
					} else {
						//else just use specified position
						pos2 = mScenarioData->sides.at(i)->units.at(j)->positions.at(k);
					}
					osg::Vec3 pos(pos2[0], pos2[1], 0);
					Unit* unit = NULL;
					switch (mScenarioData->sides.at(i)->units.at(j)->type) {
						case SOLDIER:
							oss << "Soldier" << unitIndex;
							unit = new UnitSoldier(oss.str(), mapPointToTerrain(pos), 0, mScenarioData->sides.at(i)->side, true, getInstanceID(), globalUnitIndex);
							break;
						case TANK:
							oss << "Tank" << unitIndex;
							unit = new UnitTank(oss.str(), mapPointToTerrain(pos), 0, mScenarioData->sides.at(i)->side, true, getInstanceID(), globalUnitIndex);
							break;
					}
					if (unit != NULL) {
						if (!mScenarioData->autoFire) {
							UnitFiring* u = dynamic_cast<UnitFiring*>(unit);
							if (u) u->setAutoFire(false);
						}
						AddUnit(unit);
						m_Net->RegisterInitPacket( UnitInitPacket(unit) );
						unitIndex++;
						globalUnitIndex++;
					}
				}
			} else {
				//count the remote units
				m_NumRemoteUnits++;
			}
		}
	}

	if (m_HostName.empty() && !mConfigFile.empty()) {
		//parse config file to create observers and record num of clients and hostnames
		QDomDocument doc( "Config" );

		LOG_DEBUG("Parsing config: " + mConfigFile + "...");

		std::string configFilePath = dtCore::FindFileInPathList( "scenarios/config/" + mConfigFile + ".xml");
		if (configFilePath.empty()) {
			 LOG_ERROR("Could not find config to load: " + configFilePath );
		}

		QFile file( QString(configFilePath.c_str()) );
		if( !file.open( QFile::ReadOnly ) )
		  LOG_ERROR("Cannot open config definition file: " + configFilePath);

		QString errormsg;
		int errorline;

		if( !doc.setContent( &file, &errormsg, &errorline ) )
		{
		  file.close();
		  LOG_ERROR("Cannot parse config definition file " + mConfigFile + " at line " +
			  boost::lexical_cast<std::string>(errorline) + ": " + errormsg.toStdString() );
		}
		file.close();

		QDomElement root = doc.documentElement();
		int numObservers = 0;

		if( !root.isNull() ) {
			if( root.tagName() == "config" ) {
				QDomElement e = root.firstChild().toElement();
				while (!e.isNull() ) {
					if( e.tagName() == "game" ) {
						DirectionGame* game = dynamic_cast<DirectionGame*>(mGame);
						if (game) {
							if (e.hasAttribute("segments")) {
								game->setNumSegments(e.attribute("segments").toInt());
							}
							if (e.hasAttribute("levels")) {
								game->setNumLevels(e.attribute("levels").toInt());
							}
						}
					}
					if( e.tagName() == "clients" ) {
						if (e.hasAttribute("launch")) {
							mLaunchClients = boost::lexical_cast<int>((e.attribute("launch")).toStdString());
						}
						//the number of clients to launch
						if (e.hasAttribute("num")) {
							mNumClients = e.attribute("num").toInt();
						} else {
							mNumClients = 1;
						}
						//remove default client hosts
						for (unsigned int i=0; i<mClientHosts.size(); i++)
							delete mClientHosts[i];
						mClientHosts.clear();
						QDomElement h = e.firstChild().toElement();
						while (!h.isNull() ) {
							if ( h.tagName() == "host") {
								int cores = 1;
								if (h.hasAttribute("cores")) {
									cores = h.attribute("cores").toInt();
								}
								std::string path = "~/development/workspace/hammerQt";
								if (h.hasAttribute("path")) {
									path = h.attribute("path").toStdString();
								}
								std::string exec = "hammerQt";
								if (h.hasAttribute("exec")) {
									exec = h.attribute("exec").toStdString();
								}
								mClientHosts.push_back(new HypothesisHost(h.text().toStdString(),cores, path, exec));
							}
							h = h.nextSibling().toElement();
						}
					}
					if( e.tagName() == "observers" ) {
						QDomElement o = e.firstChild().toElement();
						while (!o.isNull() ) {
							if ( o.tagName() == "observer") {
								QString observer = o.text();
								if (observer == "global") {
									mObservations->addObserver(createObserver(GLOBAL));
								} else if (observer == "round-robin") {
									mObservations->addObserver(createObserver(ROUND_ROBIN));
									numObservers++;
								} else if (observer == "sequential") {
									mObservations->addObserver(createObserver(SEQUENTIAL));
									numObservers++;
								} else if (observer == "threat") {
									mObservations->addObserver(createObserver(THREAT));
									numObservers++;
								}
							}
							o = o.nextSibling().toElement();
						}
					}
					e = e.nextSibling().toElement();
				}
			}
		}

		EventLog::GetInstance().logEvent("OBSN "+boost::lexical_cast<std::string>(numObservers));
		EventLog::GetInstance().logEvent("CLNT "+boost::lexical_cast<std::string>(mNumClients));

		/*mObservations->addObserver(createObserver(GLOBAL));
		mObservations->addObserver(createObserver(ROUND_ROBIN));
		mObservations->addObserver(createObserver(ROUND_ROBIN));
		//mObservations->addObserver(createObserver(SEQUENTIAL));
		//mObservations->addObserver(createObserver(SEQUENTIAL));
		//mObservations->addObserver(createObserver(SEQUENTIAL));*/
	}
}

/**
 * Loads the GOAL commands from a log file.
 * The commands are triggered from HTAppBase::PreFrame().
 *
 * Assumes the log file is from the same scenario that is loaded
 */
int HTAppBase::loadReplay(std::string replayFile) {
	std::string replayPath = "";
	if (!replayFile.empty()) {
		replayPath = dtCore::FindFileInPathList( "results/logs/" + replayFile );
		if (replayPath.empty()) {
			//assume an absolute filename
			replayPath = replayFile;
		}
	} else {
		LOG_ERROR("No replay file specified")
		return -1;
	}

	std::ifstream ifs(replayPath.c_str(),std::ifstream::in);
	if (!ifs.is_open()) {
		LOG_ERROR("Could not find log file to load.  Missing file is """ + replayPath + """" );
		return -1;
	}

	double timeOffset = 0;
	if (dtCore::System::GetInstance().IsRunning()) {
		timeOffset = theApp->getSimTime();
		restartScenario();
	}

	double timeStamp;
	//parse log file for GOAL commands
	while (ifs.good()) {
		std::string command;
		ifs >> timeStamp >> command;
		//check that we're loading the correct log file
		if (command == "MAP") {
			std::string mapName;
			ifs >> mapName;
			if (mapName != m_MapName) {
				LOG_ERROR("The log file loaded for replay does not match the loaded scenario. Scenario map is "+m_MapName+", but the replay map is "+mapName+".")
				return -2;
			}
		}
		else if (command == "BLUE") {
			int numUnits;
			ifs >> numUnits;
			//check num blue
			/*if (m_NumBlue != numUnits) {
				LOG_ERROR("The log file loaded for replay does not match the loaded scenario. Number of blue units in the scenario is "+boost::lexical_cast<std::string>(m_NumBlue)+", but the number in the replay is "+boost::lexical_cast<std::string>(numUnits))
				return -2;
			}*/
		}
		else if (command == "RED") {
			int numUnits;
			ifs >> numUnits;
			//check num red
			/*if (m_NumRed != numUnits) {
				LOG_ERROR("The log file loaded for replay does not match the loaded scenario. Number of red units in the scenario is "+boost::lexical_cast<std::string>(m_NumRed)+", but the number in the replay is "+boost::lexical_cast<std::string>(numUnits))
				return -2;
			}*/
		}
		//parse all the goal commands
		else if (command == "GOAL") {
			//got goal
			std::string unit;
			double posX, posY, posZ;
			ifs >> unit >> posX >> posY >> posZ;
			Unit* u = getUnitByName(unit);
			UnitObserver* o = dynamic_cast<UnitObserver*>(u);
			if (u != NULL && o == NULL) {
				osg::Vec3 pos = osg::Vec3(posX,posY,posZ);
				mReplayCommands.push_back(new ReplayCommand(timeStamp+timeOffset,u,pos));
			} else {
				if (u == NULL && o == NULL) LOG_ERROR("Unknown unit "+unit)
			}
		} else {
			//discard
			ifs.ignore(1024,'\n');
		}
	}

	//set the time limit to be the last timestamp in the log
	LOG_INFO("Setting replay time limit to "+boost::lexical_cast<std::string>(timeStamp+timeOffset));
	mRemainingTime = mScenarioData->timeLimit = timeStamp+timeOffset;

	return 0;
}

void HTAppBase::InitHypothesisManager() {
	//create hypothesis manager (creates server on this host on another port)
	if (m_HostName.empty()) {
		HypothesisManager::create(getHostname(), 4449, mTargets, mObservations, NULL);
	}
	//if not launching clients, they must be manually launched with the same ids as below
	if (!mLaunchClients) {
		for (unsigned int i=0; i < mNumClients; i++) {
			new HypothesisClient(i+100);
		}
	}
}

std::string HTAppBase::getHostname() {
	char h[255];
	if (gethostname(h, 255) == 0) {
		return std::string(h);
	} else {
		return "localhost";
	}
}

void HTAppBase::Run()
{
   dtCore::System::GetInstance().Run();
}

/**
 * Resets all the units back to their original positions
 */
void HTAppBase::restartScenario() {
	dtCore::System::GetInstance().SetPause(1);

	clearScene();
	CreateActors();

	mRemainingTime = mScenarioData->timeLimit;
	m_AttackerOnGoalTime = 0;

	dtCore::System::GetInstance().SetPause(0);
}

void HTAppBase::AddUnit(Unit* unit) {
	GetScene()->AddDrawable(unit);
	 // two-stage initialization
	 // thats because physics can only be setup after an object has been added to the scene
	unit->init();
	m_Units.insert(dtCore::RefPtr<Unit>(unit));

	//tell the inverse models that a unit has been added
	for (unsigned int i=0; i < mInverseModels.size(); i++) {
		mInverseModels.at(i)->addUnit(unit);
	}
}

void HTAppBase::RemoveUnit(Unit* unit, bool clear)
{
	UnitObserver* o = dynamic_cast<UnitObserver*>(unit);
	if (o != NULL) {
		mObservations->removeObserver(o);
	}

	//tell the inverse models that a unit has been removed
	for (std::vector<dtCore::RefPtr<InverseModel> >::iterator it=mInverseModels.begin(); it != mInverseModels.end();) {
		(*it)->removeUnit(unit, clear);
		//if all units have been removed from an IM then delete it
		if (!(*it)->valid()) {
			it = mInverseModels.erase(it);
		} else {
			it++;
		}
	}

	//tell the hypothesis manager that a unit has been removed (to keep the observer up to date)
	//HypothesisManager* m = HypothesisManager::getManager();
	//if (m != NULL) m->removeUnit(unit);

	unit_it it;

	// find it
	for (it = m_Units.begin(); it != m_Units.end(); it++)
	{
		if ((*it).get() == unit) break;
	}

	EventLog::GetInstance().logEvent("DEAD "+unit->getName());

	//remove unit from units lists (RefPtr count should then be zero)
	GetScene()->RemoveDrawable((*it).get());
	m_Units.erase(it);
}

/**
 * Override of dtABC::PreFrame
 * Called automatically before each frame is rendered
 * Used to update object properties
 */
void  HTAppBase::PreFrame( const double deltaFrameTime )
{
	//maintain a global simulation time variable
	m_SimTime += deltaFrameTime;
	m_FrameTime = deltaFrameTime;

	if (m_HostName.empty()) {
		mWatchdog->frame(m_SimTime);
	}

	// net traffic
	m_Net->PreFrame( deltaFrameTime );

	mRemainingTime -= m_FrameTime;
	signed rem_sec = (signed)mRemainingTime;//mScenarioData->timeLimit - (signed)(m_SimTime);

	//loop through all units
	bool found_attacker_on_goal = false;
    bool found_attacker = false, found_defender = false;
    signed int goal_index = m_Terrain->GetLayerIndex("goal");
    signed long int goal_mask = 1l << goal_index;

	for (unit_it it = m_Units.begin(); it != m_Units.end(); it++) {
		(*it)->update();

		// check whether any of the red ones are on the goal
		if ((*it)->getSide() == mAttackingSide) {
			found_attacker = true;
			osg::Vec3 act_pos = (*it)->getPosition();

			long unsigned int layer_bits = m_Terrain->GetLayerBitsAt(act_pos.x(), act_pos.y());

			if (layer_bits & goal_mask) {
				// it is in the goal area
				found_attacker_on_goal = true;
			}
		} else if ((*it)->getSide() == mDefendingSide) {
			found_defender = true;
		}
	}

	if (found_attacker_on_goal) {
		m_AttackerOnGoalTime += deltaFrameTime;
	} else { // reset counter
		m_AttackerOnGoalTime = 0;
	}

	//run the update fn on all active projectiles
	std::set<dtCore::RefPtr<Projectile> >::iterator projIter;
	for (projIter = Projectile::projectiles.begin(); projIter != Projectile::projectiles.end(); projIter++)
	{
		// skip if projectile already detonated
		if ( (*projIter).get()->getDetonateTime() != 0 ) continue;

		//update returns true if projectile has intersected with unit or gone below the terrain...
		if ((*projIter)->update()) {
			//get yield and position before projectile is detonated
			float projYield = (*projIter)->getYield();
			osg::Vec3 projPos = (*projIter)->getPosition();
			//boom
			(*projIter)->detonate();
			BroadcastEvent(Event::DETONATION, (*projIter)->getOwner(), NULL, (*projIter)->getPosition());
			//see how much it hurt
			for (unit_it unitIter = m_Units.begin(); unitIter != m_Units.end(); unitIter++) {
				float health;
				double hurt = 0;

				if ((*projIter)->getType() == Projectile::SHELL) {
					double dist2 = ((*unitIter)->getPosition() - projPos).length2();
					// blast radius is inverse square of yield with distance
					hurt = projYield / dist2;
					hurt *= getTerrain()->GetShellYieldAt(projPos.x(), projPos.y());

				} else if ((*projIter)->getType() == Projectile::BULLET) {
					//only get hurt from a bullet if it hit the unit directly
					if ((*projIter)->isDirectHit((*unitIter).get())) {
						hurt = projYield;
					}
					hurt *= getTerrain()->GetBulletYieldAt(projPos.x(),	projPos.y());
				}
				hurt *= 1 / (*unitIter)->getArmour();

				//limit hurt to 100
				if (hurt > 100)
					hurt = 100;

				if (hurt > 0) {
					health = (*unitIter)->decrementHealth(hurt);
					BroadcastEvent(Event::HURT, (*projIter)->getOwner(), (*unitIter).get(), osg::Vec3(hurt, NAN, NAN));
				}
			}
		}
	}

	// remove units which are dead (only in the real world sim)
	if (!isForwardModel()) {
		bool checked = false;
		while (!checked)
		{
			checked = true;
			for (unit_it it = m_Units.begin(); it != m_Units.end(); it++)
			{
				if ( (*it)->getHealth() <= 0 )
				{
					RemoveUnit((*it).get());

					//remove unit from scene
					checked = false;
					break;
				}
			}
		}
	}

	//loop through all inverse models
	for (unsigned int i=0; i < mInverseModels.size(); i++) {
		mInverseModels.at(i)->update();
	}

	//group units
	mTargets->update();

	//GAME
	if (mGame && mGame->isInitialised()) {
		mGame->update();
	} else {
		if (mScenarioData->gameType == "capture") {
			mGame->init();
		}
	}

	//-- cleanup --

	//delete projectile object and explosion object after 6 seconds
	for (projIter = Projectile::projectiles.begin(); projIter != Projectile::projectiles.end();)
	{
		if ((*projIter).get()->getDetonateTime() != 0 &&
		    (*projIter).get()->getDetonateTime() + 6 <= getSimTime())
		{
			Projectile::projectiles.erase(projIter++);
		} else {
			projIter++;
		}
	}
	//(remove collision joints)
	dJointGroupEmpty(m_ContactGroup);

	//Syncronise clients each log interval
	if (getIsServer() && EventLog::GetInstance().isTimeForNextLog()) {
		BroadcastEvent(Event::TIME_SYNC);
	}


	if (!isForwardModel()) {
		// handle game over cases

		// time is out for attackers
		if (rem_sec <= 0) {
			// assuming blue is defender
			EndGame(mDefendingSide);
		}
		// when time on goal is out for defenders
		else if (m_AttackerOnGoalTime >= mScenarioData->goalTime) {
			// assuming blue is defender
			EndGame(mAttackingSide);
		}
		// lost all units
		else if ((m_NumRemoteUnits == m_NumRemoteUnitsReceived) && !found_attacker) {
			EndGame(mDefendingSide);
		} else if ((m_NumRemoteUnits == m_NumRemoteUnitsReceived) && !found_defender) {
			EndGame(mAttackingSide);
		}
	}

	//replays
	if (!mReplayCommands.empty()) {
		if (mReplayCommandIndex < mReplayCommands.size()) {
			double simTime = theApp->getSimTime();
			ReplayCommand* c = mReplayCommands[mReplayCommandIndex];
			while (c->time <= simTime) {
				LOG_INFO("Setting a new goal for "+c->unit->getName())
				c->unit->setGoalPosition(c->pos);
				delete c;
				if (++mReplayCommandIndex == mReplayCommands.size()) {
					break;
				}
				c = mReplayCommands[mReplayCommandIndex];
			}
		}
	}

    // update terrain drawings if necessary
    m_Terrain->UpdatePrimitives();
    m_PlanCombiner->AnimatePlansDemo();

    //Observers update all units observed positions (if a server)
    if (m_HostName.empty()) mObservations->update(deltaFrameTime);

    if (mLaunch) {
		launchHypotheses();
		mLaunch = false;
	}

    //preframe updates for hypothesis manager
	HypothesisManager* m = HypothesisManager::getManager();
	if (m != NULL) m->update();

    //Preframe for subclasses to override
    PreFrameGUI(deltaFrameTime);

    //Reset log interval
    EventLog::GetInstance().intervalDone();
}

/**
 * Overridden by subclasses
 */
void HTAppBase::PreFrameGUI( const double deltaFrameTime ) {
}

Unit* HTAppBase::getUnitByName(std::string name)
{
	for (unit_it it = m_Units.begin(); it != m_Units.end(); it++)
	{
		if (name == (*it)->getName())
		{
			return (*it).get();
		}
	}
	return 0;
}

bool HTAppBase::GetTerrainHeight(double x, double y, double & h)
{
	osg::BoundingBox terrBB = m_Terrain->GetBB();

	if (terrBB.xMin() > x || terrBB.xMax() < x || terrBB.yMin() > y || terrBB.yMax() < y)
	{
		h = 0;
		return false;
	}

	h = m_Terrain->GetHeight(x,y);

	return true;
}

osg::Vec3 HTAppBase::mapPointToTerrain(osg::Vec3 p) {
	osg::BoundingBox terrBB = m_Terrain->GetBB();

	//Map points outside the BB to the nearest point within
	if (terrBB.xMin() > p.x()) {
		p.x() = terrBB.xMin();
	}
	if (terrBB.xMax() < p.x()) {
		p.x() = terrBB.xMax();
	}
	if (terrBB.yMin() > p.y()) {
		p.y() = terrBB.yMin();
	}
	if (terrBB.yMax() < p.y()) {
		p.y() = terrBB.yMax();
	}

	//clamp z
	double z;
	GetTerrainHeight(p.x(), p.y(), z);
	p.z() = z;

	return p;
}

/**
 * ODE collision callback
 *
 * This is called by dSpaceCollide when two objects in space are
 * potentially colliding.
 *
 */

void HTAppBase::nearCallback (void *data, dGeomID o1, dGeomID o2)
{
   //NOTE: mostly copied from dtCore::Scene!

   if( data == 0 || o1 == 0 || o2 == 0 )
   {
      return;
   }

   //dtCore::Scene* scene = static_cast<dtCore::Scene*>(data);
   HTAppBase *proj = static_cast<HTAppBase*>(data);

   dtCore::Transformable* c1 = static_cast<dtCore::Transformable*>( dGeomGetData(o1) );
   dtCore::Transformable* c2 = static_cast<dtCore::Transformable*>( dGeomGetData(o2) );

   //Projectiles dont collide with anything! They explode when they go below the terrain
   if ((c1->GetName() == "projectile") ||  (c2->GetName() == "projectile")) {
   		return;
   }

   dContactGeom contactGeoms[8];

   int numContacts = dCollide( o1, o2, 8, contactGeoms, sizeof(dContactGeom) );

/*   static std::map<dGeomID,int> myo;
   // if (myo.count(o1) == 0) myo.insert(std::pair<dGeomID,int>(o1,numContacts));
   myo[o1] = numContacts;

   std::map<dGeomID,int>::iterator it;

   std::ostringstream ss;
   for (it = myo.begin(); it != myo.end(); it++)
   {
   ss << (*it).second << " ";
   }
   ss << std::endl;

   theApp->hudGUI->st->setText(ss.str());

  dtUtil::Log::GetInstance().LogMessage(__FUNCTION__, __LINE__, ss.str(), dtUtil::Log::LOG_DEBUG);
*/

/*   if (o1 == myo && numContacts > 0)
   {
   std::ostringstream ss;
   ss << "o1: " << o1 << "o2: " << o2;
   ss << ", NC: " << numContacts;
   ss << ", POS: (" << contactGeoms[0].pos[0] <<"," << contactGeoms[0].pos[1] << "," << contactGeoms[0].pos[2] << ") ";
   ss << ", NORM: (" << contactGeoms[0].normal[0] <<"," << contactGeoms[0].normal[1] << "," << contactGeoms[0].normal[2] << ") ";
   ss << ", DEPTH: (" << contactGeoms[0].depth << ") " << std::endl;
  dtUtil::Log::GetInstance().LogMessage(__FUNCTION__, __LINE__, ss.str(), dtUtil::Log::LOG_DEBUG);
   }
*/

//   theApp->hudGUI->st->setText(ss.str());

//   if (numContacts >= 1) std::cout << " " << numContacts;

   if( numContacts > 0 && c1 != 0 && c2 != 0 )
   {
      dtCore::Scene::CollisionData cd;

      cd.mBodies[0] = c1;
      cd.mBodies[1] = c2;

      cd.mLocation.set(
         contactGeoms[0].pos[0], contactGeoms[0].pos[1], contactGeoms[0].pos[2]
      );

      cd.mNormal.set(
         contactGeoms[0].normal[0], contactGeoms[0].normal[1], contactGeoms[0].normal[2]
      );

      cd.mDepth = contactGeoms[0].depth;

      proj->GetScene()->SendMessage("collision", &cd);

      if( c1 != 0 || c2 != 0 )
      {
         dContact contact;

         for( int i = 0; i < numContacts; i++ )
         {
 /*			//DEFAULTS
              contact.surface.mode = dContactBounce;
              contact.surface.mu = 1000.0;
              contact.surface.bounce = 0.75;
              contact.surface.bounce_vel = 0.001;
*/
/*            contact.surface.mode = dContactBounce;
            // contact.surface.mode = 0;
            contact.surface.mu = 1000.0;
            contact.surface.bounce = 0.0;
            contact.surface.bounce_vel = 100;
*/

/*                contact.surface.mode = dContactBounce | dContactSoftCFM;
    contact.surface.mu = dInfinity;
    contact.surface.mu2 = 0;
    contact.surface.bounce = 0.1;
    contact.surface.bounce_vel = 0.1;
    contact.surface.soft_cfm = 0.01;
*/
            // contact.surface.mode = dContactSlip1 | dContactSlip2 | dContactSoftERP | dContactSoftCFM | dContactApprox1;
            // contact.surface.mode = dContactSoftERP | dContactSoftCFM;
            // contact.surface.mode = dContactMotion1 | dContactMotion2| dContactSlip1| dContactSlip2 | dContactSoftERP | dContactSoftCFM | dContactApprox1;
            // contact.surface.mode = dContactBounce | dContactMotion1 | dContactMotion2 | dContactSoftERP | dContactSoftCFM | dContactApprox1;

			// contact.surface.mode = dContactBounce | dContactSoftCFM | dContactSoftERP | dContactApprox1;
			contact.surface.mode = dContactBounce | dContactApprox1 | dContactSoftCFM | dContactFDir1 | dContactSlip2;
		    contact.surface.mu = 1;
		    contact.surface.mu2 = 0.3;
		    contact.surface.slip2 = 0.001;
		    contact.surface.bounce = 0.1;
    		contact.surface.bounce_vel = 0.1;
		    // contact.surface.soft_erp = 0.1;
		    contact.surface.soft_cfm = 1e-5;

            contact.geom = contactGeoms[i];

            //find out which c1 or c2 is a wheel, get direction of travel and assign to fdir1
            if (c1->GetName() == "Wheel") {
				dtCore::Transform transform;
				osg::Matrix mat;
				osg::Quat q;
				osg::Vec3 dir;
				c1->GetTransform(transform);
				transform.GetRotation(mat);
				mat.get(q);
				dir = q*osg::Vec3(0,1,0);

            	contact.fdir1[0] = dir[0];
            	contact.fdir1[1] = dir[1];
            	contact.fdir1[2] = dir[2];
            }
            if (c2->GetName() == "Wheel") {
				dtCore::Transform transform;
				osg::Matrix mat;
				osg::Quat q;
				osg::Vec3 dir;
				c2->GetTransform(transform);
				transform.GetRotation(mat);
				mat.get(q);
				dir = q*osg::Vec3(0,1,0);

            	contact.fdir1[0] = dir[0];
            	contact.fdir1[1] = dir[1];
            	contact.fdir1[2] = dir[2];
            }


            // Make sure to call these both, because in the case of
            // Trigger, meaningful stuff happens even if the return
            // is false.
            bool contactResult1 = c1->FilterContact(&contact, c2);
            bool contactResult2 = c2->FilterContact(&contact, c1);

            if( contactResult1 && contactResult2 )
            {
               // All this also should be in a virtual function.
               dtCore::Physical* p1 = dynamic_cast<dtCore::Physical*>(c1);
               dtCore::Physical* p2 = dynamic_cast<dtCore::Physical*>(c2);

               if( p1 != 0 || p2 != 0 )
               {
                  dJointID joint = dJointCreateContact( proj->GetScene()->GetWorldID(),proj->m_ContactGroup,&contact);

                  dJointAttach( joint,
                                p1 != 0 && p1->DynamicsEnabled() ? p1->GetBodyID() : 0,
                                p2 != 0 && p2->DynamicsEnabled() ? p2->GetBodyID() : 0 );
               }
            }
         }
      }
   }
}

bool HTAppBase::IsValidPosition(osg::Vec3 pos)
{
	double height;
	if (GetTerrainHeight(pos.x(), pos.y(), height))
		if ((float)height < pos.z()+1)
			return true;

	LOG_WARNING("Coordinate is out of map boundaries or below the terrain.");
	return false;
}

bool HTAppBase::IsValidXYPosition(float x, float y)
{
	double height;
	if (GetTerrainHeight(x, y, height)) return true;
	std::cout << x << "," << y;
	LOG_WARNING("Coordinate is out of map boundaries or below the terrain.");
	return false;
}

osg::Vec3 HTAppBase::GetRandomPosition()
{
	osg::Vec3 pos;
	osg::BoundingBox terrBB = m_Terrain->GetBB();

	pos.x() = dtUtil::RandFloat( terrBB.xMin(), terrBB.xMax() );
	pos.y() = dtUtil::RandFloat( terrBB.yMin(), terrBB.yMax() );
	double height;
	GetTerrainHeight(pos.x(), pos.y(), height);
	pos.z() = height;

	return pos;
}

osg::Vec3 HTAppBase::PreGetRandomPosition()
{
	osg::Vec3 pos;

	float edges = 0.9;

	pos.x() = dtUtil::RandFloat( -(cMapHorizontalSize/2)*edges, (cMapHorizontalSize/2)*edges );
	pos.y() = dtUtil::RandFloat( -(cMapHorizontalSize/2)*edges, (cMapHorizontalSize/2)*edges );
	pos.z() = 0;

	return pos;
}

osg::Vec2 HTAppBase::CalcMapPosFromWorldPos(osg::Vec2 world_pos) const
{
	osg::Vec2 map_pos;
	osg::BoundingBox terrBB = m_Terrain->GetBB();

	map_pos.x() = ( world_pos.x() - terrBB.xMin() ) / ( terrBB.xMax() - terrBB.xMin() );
	map_pos.y() = ( world_pos.y() - terrBB.yMin() ) / ( terrBB.yMax() - terrBB.yMin() );

	return map_pos;
}

osg::Vec2 HTAppBase::CalcWorldPosFromMapPos(osg::Vec2 map_pos) const
{
	osg::Vec2 world_pos;
	osg::BoundingBox terrBB = m_Terrain->GetBB();

	world_pos.x() = + terrBB.xMin() + map_pos.x()*( terrBB.xMax() - terrBB.xMin() );
	world_pos.y() = + terrBB.yMin() + map_pos.y()*( terrBB.yMax() - terrBB.yMin() );

	return world_pos;
}

void HTAppBase::Quit()
{
   //notify everyone else we are quitting
   PlayerQuitPacket packet( GetUniqueId().ToString() );
   m_Net->SendPacket( "all", packet );

   // wait to send
   sleep(1);

   //shutdown the networking
   m_Net->Shutdown();

   // needed otherwise it crashes on exit
   m_Units.clear();
   Projectile::projectiles.clear();

   dtABC::BaseABC::Quit();
}


void HTAppBase::CreateRemoteUnit(UnitInitPacket& packet)
{
	//count the number of remote units received
	m_NumRemoteUnitsReceived++;

	//create a representation of the remote unit
	dtCore::RefPtr<UnitRemote> unit = new UnitRemote(packet, false, packet.mClientID, packet.mUnitID);

	LOG_INFO( "Created remote unit " + packet.m_UniqueUnitID + "." )

	AddUnit(unit.get());
}

void HTAppBase::GetPositionOfNetObject(std::string ID, osg::Vec3& XYZ, osg::Vec3& HPR)
{
	m_Net->GetPositionOf(ID, XYZ, HPR);
}

void HTAppBase::SendPosition(std::string ID, osg::Vec3& XYZ, osg::Vec3& HPR)
{
   // do it in a stochastic way if needed to reduce bandwidth
   // the probability of sending is set in PreFrame()
/*   if (float(rand())/RAND_MAX < m_ProbOfSend)
   {*/
	if (!isForwardModel() && mScenarioData->numInstances > 1) {
   		PositionPacket packet( XYZ, HPR, ID );
   		m_Net->SendUnreliablePacket( "all", packet );
	}
//   		m_NumUpdates++;
//   }
}

void HTAppBase::BroadcastEvent(Event::EventType e, Unit *unit1, Unit *unit2, osg::Vec3 pos) {
	//transmit to other participants
	std::vector<std::string> participants;
	std::vector<double> data;
	osg::Vec3 vec;
	switch (e) {
		case Event::POSITION:
			participants.push_back(unit1->GetName());
			vec = unit1->getPosition();
			data.push_back(vec[0]);
			data.push_back(vec[1]);
			data.push_back(vec[2]);
			break;
		case Event::HEADING:
			participants.push_back(unit1->GetName());
			data.push_back(unit1->getHeading());
			break;
		case Event::POSITION_AND_HEADING:
			if (EventLog::GetInstance().isTimeForNextLog()) {
				participants.push_back(unit1->GetName());
				vec = unit1->getPosition();
				data.push_back(vec[0]);
				data.push_back(vec[1]);
				data.push_back(vec[2]);
				data.push_back(unit1->getHeading());
			} else {
				return;
			}
			break;
		case Event::NEW_GOAL:
			participants.push_back(unit1->GetName());
			vec = unit1->getGoalPosition();
			data.push_back(vec[0]);
			data.push_back(vec[1]);
			data.push_back(vec[2]);
			break;
		case Event::SELECTED:
			participants.push_back(unit1->GetName());
			break;
		case Event::DETONATION:
			participants.push_back(unit1->GetName());
			data.push_back(pos[0]);
			data.push_back(pos[1]);
			data.push_back(pos[2]);
			break;
		case Event::SEES:
			participants.push_back(unit1->GetName());
			participants.push_back(unit2->GetName());
			break;
		case Event::HIDE:
			participants.push_back(unit1->GetName());
			participants.push_back(unit2->GetName());
			break;
		case Event::FIRE:
			participants.push_back(unit1->GetName());
			participants.push_back(unit2->GetName());
			break;
		case Event::HURT:
			participants.push_back(unit1->GetName());
			participants.push_back(unit2->GetName());
			data.push_back(pos[0]);
			break;
		case Event::PAUSE:
			data.push_back(m_MySide);

			if (m_MySide == RED)
         	{
				data.push_back(m_RedPaused);
         	}

         	if (m_MySide == BLUE)
         	{
				data.push_back(m_BluePaused);
         	}
			break;

		case Event::GAME_OVER:
		        if (unit1 == 0)
		          data.push_back(0); // blue won
		        else
		          data.push_back(1); // red won
		        break;

		case Event::TIME_SYNC:
			//this player
			if (m_MySide == RED) {
				participants.push_back("RedPlayer ");
			} else if (m_MySide == BLUE) {
				participants.push_back("BluePlayer");
			}
			//data
			data.push_back(mRemainingTime);
			data.push_back(getTimeScale());
			break;

		case Event::TIME_SCALE:
			//nothing
			return;
		default:
			LOG_DEBUG("Unhandled event type");
			return;
	}
	if (!isForwardModel()) {
		EventPacket packet( e, participants, data );
		m_Net->SendEvent( packet );
	}
	//log
	EventLog::GetInstance().log(e, participants, data);
}

void HTAppBase::ReceiveEvent(Event::EventType event, std::vector<std::string>& participants, std::vector<double>& data) {
	if (!isForwardModel()) {
		switch (event) {
			case Event::POSITION:
				//nothing
				break;
			case Event::HEADING:
				//nothing
				break;
			case Event::POSITION_AND_HEADING:
				//nothing
				break;
			case Event::NEW_GOAL:
				//nothing
				break;
			case Event::SELECTED:
				//nothing
				break;
			case Event::DETONATION:
				//nothing
				break;
			case Event::SEES:
				//nothing
				break;
			case Event::HIDE:
				//nothing
				break;
			case Event::FIRE:
				//nothing
				break;
			case Event::HURT:
				//update health
				getUnitByName(participants.at(1))->decrementHealth(data.at(0));
				break;
			case Event::PAUSE:
				//pause sim TODO replace this with m_InstanceID
				if (data[0] == RED)	{
					m_RedPaused = data[1];
				}
				if (data[0] == BLUE) {
					m_BluePaused = data[1];
				}
				dtCore::System::GetInstance().SetPause( m_RedPaused | m_BluePaused );
				break;
			case Event::GAME_OVER:
				// do not do anything
				break;
			default:
				LOG_DEBUG("Unhandled event type");
				return;
		}
		//log event
		EventLog::GetInstance().log(event, participants, data);
	}
}

std::set<Unit*> HTAppBase::SearchVisibleTargets(UnitFiring & from_unit)
{
	std::set<Unit*> result;

	//loop through units
	for (unit_it it = m_Units.begin(); it != m_Units.end(); it++) {
		// first make sure target is not on the same side
		if (from_unit.getSide() == (*it)->getSide())
			continue;

		//get launch position of projectile
		osg::Vec3 from_pos = from_unit.getProjectileLaunchPos();

		//find out if the corners of the bounding box of the target are visible
		dtUtil::BoundingBoxVisitor bbv;
		(*it)->GetChild(0)->GetOSGNode()->accept(bbv);
		//get the 4 top corners Z,Y,X (100,101,110,111 = 4:7)
		for (int i = 4; i <= 7; i++) {
			osg::Vec3 target = bbv.mBoundingBox.corner(i);

			// first check line of sight
			if (getTerrain()->IsClearLineOfSight(from_pos, target)) {
				// get the terrain type for both positions
				std::string from_terrain_id = getTerrain()->GetLayerSensorsAt(from_pos.x(), from_pos.y());
				std::string to_terrain_id = getTerrain()->GetLayerSensorsAt(target.x(), target.y());

				// check whether the sensors can detect the target
				float sensor_range = from_unit.getSensorRangeIn(from_terrain_id);
				float visibility = (*it)->getVisibilityIn(to_terrain_id);

				// modify sensor range with visibility
				sensor_range = sensor_range * visibility;

				// decide whether it is closer than that
				if ((target - from_pos).length2() <= sensor_range * sensor_range) {
					result.insert((*it).get());
					break;
				}
			}
		}
	}

	return result;
}

/**
 * If 'point' is within a unit's bounding box, then return that unit
 */
Unit* HTAppBase::PointToUnit(osg::Vec3 point) {
	for (unit_it it = m_Units.begin(); it != m_Units.end(); it++)
	{
		//get bounding box of target unit's hull
        dtUtil::BoundingBoxVisitor bbv;
		(*it)->GetChild(0)->GetOSGNode()->accept(bbv);

		if (bbv.mBoundingBox.contains(point)) {
			return (*it).get();
		}
	}
	return NULL;
}

/**
 * If the line between start and end intersects with a units bounding box, then return that unit
 * Used by Projectile::update()
 */
Unit* HTAppBase::TrajectoryToUnit(osg::Vec3 start, osg::Vec3 end) {
	//create line segment for this trajectory
	osg::ref_ptr<osg::LineSegment> line = new osg::LineSegment(start, end);

	//see if it intersects with any of the units
	for (unit_it it = m_Units.begin(); it != m_Units.end(); it++)
	{
		//get bounding box of target unit's hull
        dtUtil::BoundingBoxVisitor bbv;
		(*it)->GetChild(0)->GetOSGNode()->accept(bbv);

		if (line->intersect(bbv.mBoundingBox)) {
			return (*it).get();
		}
	}
	return NULL;
}

std::set<Unit*> HTAppBase::GetAllUnits()
{
	std::set<Unit*> result;

	// create an unreferenced copy of the unit set
	for (unit_it it = m_Units.begin(); it != m_Units.end(); it++)
	{
		result.insert((*it).get());
	}

	return result;
}

void HTAppBase::togglePathPlanning() {
	mPathPlanning = !mPathPlanning;
}

void HTAppBase::clearScene() {
	//reset num of remote units
	m_NumRemoteUnitsReceived = 0;

	//remove all projectiles
	Projectile::projectiles.clear();
	//remove all existing units
	for (unit_it it = m_Units.begin(); it != m_Units.end(); it++) {
		RemoveUnit(it->get(), true);
	}
	//remove any inverse models
	mInverseModels.clear();

	//reset replay
	mReplayCommands.clear();
	mReplayCommandIndex = 0;
}

void HTAppBase::loadFile(std::string filename) {
	//remove all units and projectiles from the scene
	clearScene();

	std::string filePath = dtCore::FindFileInPathList( filename );
	// create and open an archive for input
    std::ifstream ifs(filePath.c_str(), std::ios::binary);
    boost::archive::xml_iarchive ia(ifs);
    ia.register_type(static_cast<UnitTank*>(NULL));
    ia.register_type(static_cast<UnitSoldier*>(NULL));
    //get scenario data
    ia >> BOOST_SERIALIZATION_NVP(mScenarioFilename);
    // read units from archive
    unsigned int numUnits;
    ia >> boost::serialization::make_nvp("NumUnits", numUnits);
    std::map<int,std::vector<Unit*> > remoteUnits;
    std::vector<Unit*> localUnits;
	for (unsigned int i=0; i < numUnits; i++) {
		Unit* unit;
		//if local deserialization of unit adds the unit to the scene
    	ia >> boost::serialization::make_nvp("m_Units", unit);
		if (isLocalInstance(unit->getClientID())) {
			//if local send out unit init packets (after sending their local units which clears the scene)
			localUnits.push_back(unit);
		} else {
			//if remote, send to remote instance
			remoteUnits[unit->getClientID()].push_back(unit);
		}
	}
	//send out UnitSavePackets to clients
	for (std::map<int,std::vector<Unit*> >::iterator it = remoteUnits.begin(); it != remoteUnits.end(); it++) {
		for (std::vector<Unit*>::iterator uit = (*it).second.begin(); uit != (*it).second.end(); uit++) {
			sendUnit(*uit, (*it).second.size(), (*uit)->getClientID());
			//TODO delete unit that we've just sent because its not needed anymore
			//delete *(uit);
		}
	}
	//send out UnitInitPackets to clients
	for (std::vector<Unit*>::iterator it = localUnits.begin(); it != localUnits.end(); it++) {
		m_Net->RegisterInitPacket( UnitInitPacket(*it) );
	}

	//load projectiles
	unsigned int numProjectiles;
	ia >> boost::serialization::make_nvp("NumProjectiles", numProjectiles);
	for (unsigned int i=0; i < numProjectiles; i++) {
		Projectile *p;
		ia >>  boost::serialization::make_nvp("Projectile", p);
	}

    // archive and stream closed when destructors are called
}

void HTAppBase::saveFile(std::string filename) {
	// create and open a character archive for output
    std::ofstream ofs(filename.c_str());

    // save data to archive
    boost::archive::xml_oarchive oa(ofs);
	oa.register_type(static_cast<UnitTank*>(NULL));
	oa.register_type(static_cast<UnitSoldier*>(NULL));
    //save scenario data
    oa << BOOST_SERIALIZATION_NVP(mScenarioFilename);
    // write units to archive
    unsigned int numUnits = m_Units.size();
    oa << boost::serialization::make_nvp("NumUnits", numUnits);
    //Serialise local units
    for (unit_it it=m_Units.begin(); it != m_Units.end(); it++) {
    	//Serialise local units
    	const Unit* const unit = it->get();
    	if (!unit->getIsRemote()) {
    		oa << boost::serialization::make_nvp("m_Units", unit);
    	}
    }
    //Serialise remote units
    if (mScenarioData->numInstances > 1) {
    	//for each client
    	std::set<int> clients = m_Net->GetClients();
    	for (std::set<int>::iterator it = clients.begin(); it != clients.end(); it++) {
    		//send out request packet and wait for all responses...
    		std::vector<std::string> units = m_Net->getRemoteUnits(*it);
    		for (std::vector<std::string>::iterator uit = units.begin(); uit != units.end(); uit++) {
    			const std::string str = *uit;
    			//deserialize...
    			std::istringstream iss(str);
    			boost::archive::binary_iarchive ia(iss);
				ia.register_type(static_cast<UnitTank*>(NULL));
				ia.register_type(static_cast<UnitSoldier*>(NULL));
    			Unit *unit;
    			ia >> unit;
    			//and reserialize...
    			const Unit *const u = unit;
    			oa << boost::serialization::make_nvp("m_Units", u);
    		}
    	}
    }

    //save projectiles (dont save detonated projectiles)
    int numProjectiles = 0;
    std::set<dtCore::RefPtr<Projectile> >::iterator projIter;
    for (projIter = Projectile::projectiles.begin(); projIter != Projectile::projectiles.end(); projIter++) {
		if (projIter->get()->getDetonateTime() == 0) {
			numProjectiles++;
		}
    }
    oa << boost::serialization::make_nvp("NumProjectiles", numProjectiles);
    for (projIter = Projectile::projectiles.begin(); projIter != Projectile::projectiles.end(); projIter++) {
		if (projIter->get()->getDetonateTime() == 0) {
			const Projectile *const p = projIter->get();
			oa << boost::serialization::make_nvp("Projectile", p);
		}
	}
    //TODO save remote projectiles
    // archive and stream closed when destructors are called
}

/**
 * Send out all local unit instances
 */
void HTAppBase::sendLocalUnits(int client) {
	//get the number of units we're sending
	int count = 0;
	for (unit_it it = m_Units.begin(); it != m_Units.end(); it++) {
		if (isLocalInstance((*it)->getClientID())) {
			count++;
		}
	}
	//send the units
	for (unit_it it = m_Units.begin(); it != m_Units.end(); it++) {
		if (isLocalInstance((*it)->getClientID())) {
			sendUnit((*it).get(), count, client);
		}
	}
}

void HTAppBase::sendUnit(Unit *unit, int total, int client) {
	//LOG_INFO("Sending unit");
	UnitSavePacket p = UnitSavePacket(unit, total);
	m_Net->SendPacketToClient(client, p);
}

bool HTAppBase::isLocalInstance(int id)
{
	return ((m_InstanceID == id) || isForwardModel());
}

//sends the current world state to a client over the network
void HTAppBase::sendWorldState(int client) {
	//send all units
	unsigned int count = m_Units.size();
	//local units
	for (unit_it it = m_Units.begin(); it != m_Units.end(); it++) {
		if (isLocalInstance((*it)->getClientID())) {
			//local unit
			sendUnit((*it).get(), count, client);
		}
	}
	if (m_Net->GetIsServer()) {
		//remote units
		std::set<int> clients = m_Net->GetClients();
		for (std::set<int>::iterator it = clients.begin(); it != clients.end(); it++) {
			//send out request packet and wait for all responses...
			//LOG_INFO("Requesting units from client");
			std::vector<std::string> units = m_Net->getRemoteUnits(*it);
			for (std::vector<std::string>::iterator uit = units.begin(); uit != units.end(); uit++) {
				//serialized remote unit
				const std::string str = *uit;
				UnitSavePacket p = UnitSavePacket(str, count, *it);
				m_Net->SendPacketToClient(client, p);
			}
		}
	}
	//TODO projectiles
}

bool HTAppBase::isForwardModel() {
	return m_ForwardModel;
}

void HTAppBase::loadWorldState(std::vector<std::string> *units) {
	for (std::vector<std::string>::iterator it = units->begin(); it != units->end(); it++) {
		//unit is added to the scene during deserialisation if it is a local unit
		Unit *unit = deserialiseUnit(*it);
		unit->setClientID(getInstanceID());
		//tell other clients
		if (!isForwardModel()) {
			m_Net->RegisterInitPacket( UnitInitPacket(unit) );
		}
	}
	//TODO projectiles
}

Unit *HTAppBase::deserialiseUnit(std::string unitStr, bool prediction) {
	std::istringstream iss(unitStr);
	boost::archive::binary_iarchive ia(iss);
	ia.register_type(static_cast<UnitTank*> (NULL));
	ia.register_type(static_cast<UnitSoldier*> (NULL));
	Unit *unit;
	if (prediction) {
		mAddingPrediction = true;
		//unit is not added to the scene
		ia >> unit;
		mAddingPrediction = false;
	} else {
		//unit is added to the scene during deserialisation if it is a local unit
		ia >> unit;
	}
	return unit;
}

bool HTAppBase::hasController() {
	return false;//((m_ControllerId >= 0) && (m_Net->GetConnection(m_ControllerId)));
}

bool HTAppBase::getIsServer() {
	return m_Net->GetIsServer();
}

//Is the app running with or without graphics?
bool HTAppBase::isHeadless() {
	//override this when running with graphics
	return true;
}

bool HTAppBase::unitExists(Unit* unit) {
	for (unit_it it = m_Units.begin(); it != m_Units.end(); it++)
	{
		if (unit == it->get())
		{
			return true;
		}
	}
	return false;
}

Unit* HTAppBase::getUnitById(int id) {
	for (unit_it it = m_Units.begin(); it != m_Units.end(); it++)
	{
		if (id == (*it)->getUnitID())
		{
			return (*it).get();
		}
	}
	return NULL;
}

std::vector<Unit*> HTAppBase::getUnitsByIds(std::vector<int>& ids) {
	std::vector<Unit*> units;
	for (unsigned int i=0; i < ids.size(); i++) {
		Unit* unit = getUnitById(ids[i]);
		if (unit) units.push_back(unit);
	}
	return units;
}

void HTAppBase::addInverseModel(InverseModel* model) {
	mInverseModels.push_back(model);
}

void HTAppBase::EndGame( UnitSide winner ) {
	//save results
	if (HypothesisManager::get()) saveAll();

	if (isHeadless()) dtCore::System::GetInstance().Stop();

    // turn off pause
	m_PauseEnabled = false;

	if (winner == BLUE) {
		BroadcastEvent(Event::GAME_OVER, 0);
	} else {
		// ugly, but we need to signal the winner side only
		BroadcastEvent(Event::GAME_OVER, reinterpret_cast<Unit*>(1) );
	}

	//stop or pause depending on whether we have a gui
	EndGameGUI();
}

void HTAppBase::EndGameGUI() {
	//stop the game
	dtCore::System::GetInstance().Stop();
}

void HTAppBase::setTimeScale(double timescale) {
	dtCore::System::GetInstance().SetTimeScale(timescale);
}

double HTAppBase::getTimeScale() {
	return dtCore::System::GetInstance().GetTimeScale();
}

void HTAppBase::multiplyTimeScale(double scale) {
	dtCore::System& sys = dtCore::System::GetInstance();
	sys.SetTimeScale(sys.GetTimeScale() * scale);
}

bool HTAppBase::isAddingPrediction() {
	return mAddingPrediction;
}

double HTAppBase::getRandomGaussian(double sigma) {
	return gsl_ran_gaussian(mRng, sigma);
}

/**
 * Used for log filenames. Format yyyyMMdd-hh:mm:ss
 */
std::string HTAppBase::getStartTimeString() {
	return mStartTimeStr;
}

/**
 * Get a units' name even if it is dead
 */
std::string HTAppBase::getUnitName(int id) {
	return mUnitNames[id];
}

void HTAppBase::setUnitName(int id, std::string name) {
	mUnitNames[id] = name;
}

/************ Hypothesis stuff ******************/

void HTAppBase::launchHypotheses() {
	HypothesisManager* manager = HypothesisManager::get();
	if (manager) {
		LOG_INFO("Starting hypotheses")
		if (mLaunchClients) {
			unsigned int clientNum = 0;
			unsigned int hostNum = 0;
			int coreNum = 0;
			while (clientNum < mNumClients) {
				std::cout << "Client "<<clientNum<<" host: "<<hostNum<<" core: "<<coreNum<<std::endl;
				new HypothesisClient("Client"+boost::lexical_cast<std::string>(clientNum+1), mClientHosts[hostNum]);
				if (coreNum+1 >= mClientHosts[hostNum]->getNumCores()) {
					coreNum = 0;
					if (hostNum+1 >= mClientHosts.size()) {
						hostNum = 0;
					} else {
						hostNum++;
					}
				} else {
					coreNum++;
				}
				++clientNum;
			}
			//launch them
			HypothesisClient::launchAllClients();
		}
		//start game with the initialised hypotheses
		if (mGame) {
			if (isReplay()) {
				mGame->setReplay(mReplayFile);
			}
			mGame->init(manager);
		}
	}
}

UnitObserver* HTAppBase::createObserver(UnitObserverType type) {
	//create observer
	std::ostringstream oss;
	oss << "Observer" << m_Units.size();
	osg::Vec3 startPos = mapPointToTerrain(osg::Vec3(0,0,0));
	if (type == GLOBAL) {
		startPos.z() += 7000;
	} else {
		startPos.z() += 500;
	}
	UnitObserver* o = new UnitObserver(type, oss.str(), startPos, m_Units.size());
	GetScene()->AddDrawable(o);
	o->init();
	m_Units.insert(dtCore::RefPtr<Unit>(o));
	return o;
}

void HTAppBase::saveAll() {
	std::ofstream f;
	std::string filename("results/logs/" + getStartTimeString() + "-confidence.txt");
	f.open(filename.c_str());
	std::vector<dtCore::RefPtr<Hypothesis> > hypotheses = HypothesisManager::getHypotheses();
	for (std::vector<dtCore::RefPtr<Hypothesis> >::iterator it = hypotheses.begin(); it != hypotheses.end(); it++) {
		f << (*it)->getResultsString();
		f << std::endl;
	}
	f.close();
	LOG_INFO("Results written to "+filename)
}

void HTAppBase::triggerLaunchHypotheses() {
	mLaunch = true;
}

float HTAppBase::getObservationDuration() {
	return mScenarioData->simTime;
}
float HTAppBase::getSpeedup() {
	return mScenarioData->speedup;
}
