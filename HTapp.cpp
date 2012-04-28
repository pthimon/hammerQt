#include "common.h"

#pragma GCC diagnostic ignored "-Wextra"

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
#include <dtCore/skybox.h>
#include <dtCore/infiniteterrain.h>
#include <dtCore/isector.h>
#if DELTA3D_VER < 23
#include <dtCore/boundingboxvisitor.h>
#endif
#include <dtCore/transform.h>
#include <dtABC/application.h>
#include <dtUtil/mathdefines.h>
#include <dtNet/dtnet.h>
#include <osg/Math>
#include <osgDB/FileUtils>
#include <osgDB/ReadFile>
#include <osgUtil/IntersectionVisitor>
#include <osgUtil/LineSegmentIntersector>
#include <osgViewer/Viewer>
#include <osgViewer/api/X11/GraphicsWindowX11>
#include <ode/ode.h>
#include <cassert>
#include <queue>
#include <sstream>
#include <cstdlib>
#include <map>
#include <gnelib/GNEDebug.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/xf86vmode.h>

//#include "scene_HT.h"
#include "common.h"
#include "HTapp.h"
#include "mynetwork.h"
#include "packets.h"
#include "units/UnitRemote.h"
#include "units/UnitTank.h"
#include "units/UnitSoldier.h"
#include "units/UnitObserver.h"
#include "eventlog.h"
#include "ScenarioData.h"
#include "ScenarioHandler.h"
#include "models/InverseModelPath.h"
#include "models/InverseModelAttack.h"
#include "models/InverseModelRetreat.h"
#include "models/InverseModelDefend.h"
#include "models/InverseModelPincer.h"
#include "models/InverseModelConvoy.h"

#include <dtUtil/xercesparser.h>
#include <dtUtil/xercesutils.h>
#include <xercesc/util/XMLString.hpp>

#pragma GCC diagnostic warning "-Wextra"

const float cMapHorizontalSize = 5000.0f;

//////////////////////////////////////////////////////////////////////////
HTApplication::HTApplication( const std::string& configFilename,
		const std::string& hostName, int port, bool wait_for_client, const std::string& scenarioFilename, int id, std::string& replayFile, std::string& configFile)
	: Application_HT(configFilename, hostName, port, wait_for_client, scenarioFilename, id, replayFile, configFile),
      m_MouseListener( new dtCore::GenericMouseListener()),
      m_MouseOverUnit(NULL),
      mPressed(false),
	  mDragging(false),
   	  m_RightDeselect(false),
   	  mRequestedTimeScale(1),
   	  mDesiredTimeScale(1),
   	  mCameraLookAtUnit(NULL)
{
   	dtCore::System::GetInstance().SetShutdownOnWindowClose(true);

    GetKeyboardListener()->SetReleasedCallback(dtCore::GenericKeyboardListener::CallbackType(this,&HTApplication::KeyReleased));
    GetMouse()->AddMouseListener( m_MouseListener.get() );
   	m_MouseListener->SetMovedCallback(dtCore::GenericMouseListener::MovementCallbackType(this, &HTApplication::MouseMoved));
   	m_MouseListener->SetReleasedCallback(dtCore::GenericMouseListener::ButtonCallbackType(this, &HTApplication::MouseButtonReleased));
   	m_MouseListener->SetPressedCallback(dtCore::GenericMouseListener::ButtonCallbackType(this, &HTApplication::MouseButtonPressed));
	m_MouseListener->SetClickedCallback(dtCore::GenericMouseListener::ClickCallbackType(this, &HTApplication::MouseButtonClicked));
	m_MouseListener->SetDraggedCallback(dtCore::GenericMouseListener::MovementCallbackType(this, &HTApplication::MouseDragged));
}

HTApplication::~HTApplication()
{
}

   //////////////////////////////////////////////////////////////////////////
void HTApplication::CreateEnvironment()
{
	HTAppBase::CreateEnvironment();

  	//effects

  	int num_effects = m_Environment->GetNumEffects();
  	dtCore::EnvEffect * myeffect;
  	LOG_DEBUG("Removing effects...");
  	for (int i = 0; i<num_effects; i++)
  	{
	  	myeffect = m_Environment->GetEffect(i);
  		m_Environment->RemEffect(myeffect);
  	}

  	LOG_DEBUG("Adding effects...");

  	//dtCore::RefPtr<dtCore::CloudDome> cd = new dtCore::CloudDome(6, 2, 0.7, 0.5, 0.7, 5, 5500.f, 20);
  	//m_Environment->AddEffect(cd.get());
  	//dtCore::RefPtr<dtCore::SkyDome> sd = new dtCore::SkyDome();
  	//m_Environment->AddEffect(sd.get());

        dtCore::RefPtr<dtCore::SkyBox> box = new dtCore::SkyBox("skyBox");
        box->SetTexture( dtCore::SkyBox::SKYBOX_RIGHT, "Textures/skybox0001.png" );
        box->SetTexture( dtCore::SkyBox::SKYBOX_LEFT, "Textures/skybox0002.png");
        box->SetTexture( dtCore::SkyBox::SKYBOX_FRONT, "Textures/skybox0003.png");
        box->SetTexture( dtCore::SkyBox::SKYBOX_BACK, "Textures/skybox0004.png");
        box->SetTexture( dtCore::SkyBox::SKYBOX_TOP, "Textures/skybox0005.png");
        box->SetTexture( dtCore::SkyBox::SKYBOX_BOTTOM, "Textures/skybox0006.png");
        m_Environment->AddEffect(box.get());

  	num_effects = m_Environment->GetNumEffects();
  	std::ostringstream ss;
  	ss << num_effects;
  	LOG_DEBUG("Num of environment effects active: "+ ss.str());
  	for (int i = 0; i<num_effects; i++)
  	{
	  	myeffect = m_Environment->GetEffect(i);
    	LOG_DEBUG("Effect name: "+ myeffect->GetName());
  	}
}

void HTApplication::Config()
{
	Application_HT::Config();

	GetWindow()->SetWindowTitle("HAMMERTIMME");

	//////////////////////////////////////////////////////////////////////////
	// Setup the motion model and the camera
	m_MotionModel = new MapMotionModel(this, GetKeyboard(),GetMouse(), GetScene());
	m_MotionModel->SetTarget(GetCamera());

	// position camera
	double startx = 0.0f, starty = -10.0f;
	double start_height;
	if (GetTerrainHeight(startx, starty, start_height)) {
		std::ostringstream ss;
		ss << "Terrain height at start position is: " << start_height;
		LOG_DEBUG(ss.str());
	} else {
		LOG_ERROR("Terrain was not found at start position.");
		start_height = 0;
	}

	dtCore::Transform tx;
#if DELTA3D_VER >= 23
        tx.Set(startx, starty, start_height+40, startx, starty+10,
                        start_height+30, 0, 0, 1);
#else
	tx.SetLookAt(startx, starty, start_height+40, startx, starty+10,
			start_height+30, 0, 0, 1);
#endif
	GetCamera()->SetTransform(tx);

	//setup GUI
	initGUI();
}

void HTApplication::initGUI() {
	// setup GUI
	hudGUI = new GUIHUD(this, GetWindow(), m_Terrain->GetMinimapFileName());
	GetScene()->AddDrawable(hudGUI->GetGUIDrawable().get());
}

void HTApplication::AddUnit(Unit* unit) {
	HTAppBase::AddUnit(unit);

	// create GUI stuff for the unit
	MarkerType mymarker = NULL;
	if ( unit->getSide() == BLUE )
	{
		mymarker = hudGUI->CreateMarker("bluemarker.tga", 0);
	}
	else if ( unit->getSide() == RED ) // done that because later it can be more colors
	{
        mymarker = hudGUI->CreateMarker("redmarker.tga", 0);
	}

	if (mymarker) unit->setMarker(mymarker);
	// add status window
	hudGUI->createUnitInfoDisplay(unit);
	displayUnitHealth(unit, unit->getHealth());
}

void HTApplication::RemoveUnit(Unit* unit, bool clear)
{
	// remove its marker on the map if exists
	hudGUI->RemoveMarker( unit->getMarker() );

	std::vector< dtCore::RefPtr<Unit> >::iterator selected_it;
	for (selected_it = m_SelectedUnits.begin(); selected_it != m_SelectedUnits.end(); selected_it++)
	{
		if ((*selected_it).get() == unit) break;
	}
	if (selected_it != m_SelectedUnits.end()) m_SelectedUnits.erase(selected_it);

	HTAppBase::RemoveUnit(unit);
}

//////////////////////////////////////////////////////////////////////////
bool HTApplication::KeyPressed(const dtCore::Keyboard *keyBoard, int kc)
{
   bool verdict(false);
   static bool wireFrame = false;
   osg::Vec3 p;
   switch (kc)
   {
   case osgGA::GUIEventAdapter::KEY_Return:
      ShowStatistics();
      verdict = true;
      break;

   case 'w':
      wireFrame = !wireFrame;
      if (wireFrame)
         GetScene()->SetRenderState(dtCore::Scene::FRONT_AND_BACK, dtCore::Scene::LINE);
      else
         GetScene()->SetRenderState(dtCore::Scene::FRONT, dtCore::Scene::FILL);

      verdict = true;
      break;

      case osgGA::GUIEventAdapter::KEY_Space:

        if (m_PauseEnabled)
          {

           if (m_MySide == RED)
           {
           	m_RedPaused = !m_RedPaused;
           	hudGUI->ShowPausedRed(m_RedPaused);
           }

           if (m_MySide == BLUE)
           {
           	m_BluePaused = !m_BluePaused;
           	hudGUI->ShowPausedBlue(m_BluePaused);
           }

           dtCore::System::GetInstance().SetPause( m_RedPaused | m_BluePaused );

           BroadcastEvent(Event::PAUSE);
          }

         verdict = true;
         break;

      case 'f':
   		 //m_SelectedUnit->fire(osg::Vec3(0,1,0.001), 100);
  		 /*if (m_SelectedUnits.size() > 0) {
			UnitFiring* my_unitfiring = dynamic_cast<UnitFiring*>(m_SelectedUnits[0].get());
			if (my_unitfiring != 0)
			{
	      		p = my_unitfiring->getPosition();
   			p[1] += 20;
	      		my_unitfiring->launch(p);
			}
		 }*/
		 break;

	  case 'p':
      //send a "ping" packet for latency info
      {
	         GNE::PingPacket ping;
	         m_Net->SendPacket("all", ping);
	         verdict = true;
      }
      break;

	  case ']':
		  requestMultiplyTimeScale(2);
		  verdict = true;
	  break;
	  case '[':
		  requestMultiplyTimeScale(0.5);
		  verdict = true;
	  break;

   case osgGA::GUIEventAdapter::KEY_Control_L:
      m_KeyModifier = osgGA::GUIEventAdapter::KEY_Control_L;
   	 break;

   default:
      verdict = Application_HT::KeyPressed(keyBoard,kc);
      break;
   }

   return verdict;
}

//////////////////////////////////////////////////////////////////////////
bool HTApplication::KeyReleased(const dtCore::Keyboard *keyBoard, int kc)
{
   bool handled(false);
   switch (kc)
   {
   	case osgGA::GUIEventAdapter::KEY_Control_L:
   		m_KeyModifier = 0;
   		break;
   default:
      break;
   }

   return handled;
}

//////////////////////////////////////////////////////////////////////////
void HTApplication::ShowStatistics()
{
	SetNextStatisticsType();
}

void HTApplication::Quit()
{
   m_SelectedUnits.clear();

	HTAppBase::Quit();
   //Application_HT::Quit();
}

void HTApplication::PreFrameGUI( const double deltaFrameTime ) {
	HTAppBase::PreFrameGUI(deltaFrameTime);

	//loop through all units
	for (unit_it it = m_Units.begin(); it != m_Units.end(); it++)
	{
		// draw unit marker if it has one
		MarkerType unit_marker = (*it)->getMarker();
		if (unit_marker != 0)
		{
			osg::Vec3 unit_pos = (*it)->getPosition();
			osg::Vec2 world_pos(unit_pos.x(), unit_pos.y());
			osg::Vec2 map_pos = CalcMapPosFromWorldPos(world_pos);
			hudGUI->SetPosOfMarker(unit_marker,map_pos.x(),map_pos.y());
		}
	}

	//update motion model
	m_MotionModel.get()->update();

	if (mCameraLookAtUnit != NULL && unitExists(mCameraLookAtUnit)) {
		moveCameraToLookAt( mCameraLookAtUnit->getPosition() );
	}

    // show timer
	if (isForwardModel()) {
		//show the FM simulation command time remaining if we are showing the GUI
		hudGUI->SetTime(getNetwork()->getCommandTimeRemaining(), floor(m_AttackerOnGoalTime));
	} else {
		hudGUI->SetTime((signed)mRemainingTime, floor(m_AttackerOnGoalTime));
	}
}

/**
 * Override of dtABC::PreFrame
 * Called automatically before each frame is rendered
 * Used to update object properties
 */
void  HTApplication::PreFrame( const double deltaFrameTime )
{
	HTAppBase::PreFrame(deltaFrameTime);
}

void HTApplication::PostFrame( const double deltaFrameTime ) {
	HTAppBase::PostFrame(deltaFrameTime);

	if (mDragging) {
		osg::Vec2 pressPoint, dragPoint;
		if (GetWindow()->CalcPixelCoords(mPressPoint[0], -mPressPoint[1], pressPoint[0], pressPoint[1])
				&& GetWindow()->CalcPixelCoords(mDragPoint[0], -mDragPoint[1], dragPoint[0], dragPoint[1])) {
			drawDragRect(pressPoint, dragPoint);
		}
	}
}

void HTApplication::drawDragRect(osg::Vec2 pressPoint, osg::Vec2 dragPoint) {
	//draw a rectangle using the Xlib API (crazy i know)
	//Display* dpy = GetWindow()->GetRenderSurface()->getDisplay();
	//Window w = GetWindow()->GetRenderSurface()->getWindow();
	osgViewer::GraphicsWindowX11* win = dynamic_cast<osgViewer::GraphicsWindowX11*>(GetWindow()->GetOsgViewerGraphicsWindow());
	if (win) {
		Display* dpy = win->getDisplay();
		Window w = win->getWindow();

		int blackColor = BlackPixel(dpy, DefaultScreen(dpy));
		//int whiteColor = WhitePixel(dpy, DefaultScreen(dpy));

		GC gc = XCreateGC(dpy, w, 0, NULL);
		XSetForeground(dpy, gc, blackColor);

		//get start drag coords
		int sx = (int)pressPoint[0];
		int sy = (int)pressPoint[1];

		//get current end drag coords
		int tx = (int)dragPoint[0];
		int ty = (int)dragPoint[1];

		int width = (sx-tx);
		int height = (sy-ty);

		//annoyingly width and height cannot be negative, so decide on the starting point here
		int x,y;
		if (width < 0) {
			x = (int)sx;
		} else {
			x = (int)tx;
		}
		if (height < 0) {
			y = (int)sy;
		} else {
			y = (int)ty;
		}
		//draw the rectangle
		XDrawRectangle(dpy, w, gc, x, y, abs(width), abs(height));
		// Send the request to the server
		XFlush(dpy);
	}
}

bool HTApplication::MouseMoved(const dtCore::Mouse* mouse, float x, float y) {
	m_MouseX = x;
	m_MouseY = y;
	m_MousePosition = terrainHitTest(x,y);

	hudGUI->setHitPos(m_MousePosition);
	/*if (m_SelectedUnit != NULL) {
		m_SelectedUnit->setTargetPosition(m_MousePosition);
		m_SelectedUnit->aim();
	}*/

	return true;
}

bool HTApplication::MouseButtonClicked(const dtCore::Mouse* mouse, dtCore::Mouse::MouseButton button, int click_count)
{
	if (button == dtCore::Mouse::RightButton)
	{
		// deselect every unit
		m_SelectedUnits.clear();

		for (unit_it it = m_Units.begin(); it != m_Units.end(); it++)
		{
			// skip if user can not control this unit
			if (!(*it)->getIsControlled()) continue;

			(*it)->setSelected(false);
		}
		return true;
	}

	return false;
}

bool HTApplication::MouseButtonPressed(const dtCore::Mouse* mouse, dtCore::Mouse::MouseButton button) {
	if (button == dtCore::Mouse::LeftButton) {
		//std::cout << "pressed " << std::endl;
		mPressed = true;
		float x, y;
		mouse->GetPosition(x, y);
		mPressPoint = osg::Vec2(x, y);
		return true;
	}
	else if (button == dtCore::Mouse::RightButton)
	{
		m_RightDeselect = true;
		return true;
	}

	return false;
}

bool HTApplication::MouseButtonReleased(const dtCore::Mouse* mouse, dtCore::Mouse::MouseButton button) {
	m_MouseOverUnit = unitHitTest(m_MouseX, m_MouseY);

	if (button == dtCore::Mouse::LeftButton) {
		if (mDragging) {
			//std::cout << "end drag " << std::endl;
			mDragging = false;
			//find corners of quadrilateral
			osg::Vec3 A = terrainHitTest(mPressPoint[0], mPressPoint[1]);
			osg::Vec3 B = terrainHitTest(mDragPoint[0], mPressPoint[1]);
			osg::Vec3 D = terrainHitTest(mPressPoint[0], mDragPoint[1]);
			osg::Vec3 C = terrainHitTest(mDragPoint[0], mDragPoint[1]);
			//make in the z=0 plane (not sure if this is necessary!)
			A[2] = 0;
			B[2] = 0;
			C[2] = 0;
			D[2] = 0;
			//make vectors
			osg::Vec3 AB = B-A;
			osg::Vec3 BC = C-B;
			osg::Vec3 CD = D-C;
			osg::Vec3 DA = A-D;

			//empty selected units list
			m_SelectedUnits.clear();

			std::ostringstream oss;
			for (unit_it it = m_Units.begin(); it != m_Units.end(); it++)
			{
				// skip if user can not control this unit
				if (!(*it)->getIsControlled()) continue;

				//point to test
				osg::Vec3 P = (*it)->getPosition();
				P[2] = 0;
				//select tanks within quadrilateral
				osg::Vec3 dp1 = (P-A) ^ AB;
				osg::Vec3 dp2 = (P-B) ^ BC;
				osg::Vec3 dp3 = (P-C) ^ CD;
				osg::Vec3 dp4 = (P-D) ^ DA;
				//oss << dp1[2] << " : " << dp2[2] << " : " << dp3[2] << " : " << dp4[2] << std::endl;
				if ((dp1[2] > 0 && dp2[2] > 0 && dp3[2] > 0 && dp4[2] > 0) || (dp1[2] < 0 && dp2[2] < 0 && dp3[2] < 0 && dp4[2] < 0)) {
					//in quadrilateral
					(*it)->setSelected(true);
					//add to selection vector
					m_SelectedUnits.push_back((*it).get());
					//oss << m_Units[i]->GetName() << std::endl;
				} else {
					(*it)->setSelected(false);
				}
			}
			//hudGUI->st->setText(oss.str());
		}
		else if (m_MouseOverUnit != NULL && m_KeyModifier != osgGA::GUIEventAdapter::KEY_Control_L)
		{
			if (m_MouseOverUnit->getIsControlled())
							setSelectedUnit(m_MouseOverUnit);
		}
		else if (m_SelectedUnits.size() > 0) {
			/*if (m_KeyModifier == Producer::Key_Control_L) {
				//all units fire! TODO should I bother to make this work again? (they just fire in the current direction of the turret)
				for (unsigned int i=0; i < m_SelectedUnits.size(); i++) {
					UnitFiring* my_unitfiring = dynamic_cast<UnitFiring*>(m_SelectedUnits[i].get());
					if (my_unitfiring != 0)
					{
						my_unitfiring->setTargetPosition(m_MousePosition);
						my_unitfiring->aim();
						my_unitfiring->fire();
					}
				}
			} else {*/
				//calc average point
				osg::Vec3 total = osg::Vec3(0,0,0);
				for (unsigned int i=0; i < m_SelectedUnits.size(); i++) {
					total += m_SelectedUnits[i]->getPosition();
				}
				osg::Vec3 averagePoint = total / m_SelectedUnits.size();
				//set goal position to mouse + offset from average point
				for (unsigned int i=0; i < m_SelectedUnits.size(); i++) {
					osg::Vec3 goalPos = m_MousePosition + (m_SelectedUnits[i]->getPosition() - averagePoint);
					if  (m_KeyModifier == osgGA::GUIEventAdapter::KEY_Control_L) {
						if (mPathPlanning) {
							InverseModelPath::createPath(m_SelectedUnits[i].get(), mapPointToTerrain(goalPos), false, true);
						} else {
							m_SelectedUnits[i]->addGoalPosition(mapPointToTerrain(goalPos));
						}
					} else {
						if (mPathPlanning) {
							InverseModelPath::createPath(m_SelectedUnits[i].get(), mapPointToTerrain(goalPos), true, true);
						} else {
							m_SelectedUnits[i]->setGoalPosition(mapPointToTerrain(goalPos));
						}
					}
				}
			//}
		}
		//std::cout << "released " << std::endl;
		mPressed = false;

		return true;
	}
	else if (button == dtCore::Mouse::RightButton && m_RightDeselect)
	{
		// deselect every unit
		m_SelectedUnits.clear();

		for (unit_it it = m_Units.begin(); it != m_Units.end(); it++)
		{
			// skip if user can not control this unit
			if (!(*it)->getIsControlled()) continue;

			(*it)->setSelected(false);
		}
		return true;
	}

	return false;
}

bool HTApplication::MouseDragged(const dtCore::Mouse* mouse, float x, float y) {
	if (mPressed) {
		mDragPoint = osg::Vec2(x,y);
		if (!mDragging) {
			if ((mDragPoint - mPressPoint).length() > 0.1) {
				//std::cout << "start drag " << std::endl;
				mDragging = true;
			}
		}
		return true;
	}

	m_RightDeselect = false;

	return false;
}

Unit *HTApplication::unitHitTest(float x, float y) {
	osgUtil::LineSegmentIntersector* picker;
    // remap the mouse x,y into viewport coordinates.
    osg::Viewport* viewport = GetCamera()->GetOSGCamera()->getViewport();
    float mx = viewport->x() + (int)((float)viewport->width()*(x*0.5f+0.5f));
    float my = viewport->y() + (int)((float)viewport->height()*(y*0.5f+0.5f));
    picker = new osgUtil::LineSegmentIntersector( osgUtil::Intersector::WINDOW, mx, my );
    osgUtil::IntersectionVisitor iv(picker);

    GetCamera()->GetOSGCamera()->accept(iv);

    if (picker->containsIntersections())
    {
    	osgUtil::LineSegmentIntersector::Intersections& intersections = picker->getIntersections();

    	for (osgUtil::LineSegmentIntersector::Intersections::iterator it=intersections.begin(); it!=intersections.end(); ++it) {
			osg::NodePath nodePath = it->nodePath;
	         for(  osg::NodePath::iterator nodeIter = nodePath.begin(); nodeIter != nodePath.end(); ++nodeIter ) {
				//loop through all units to see if this is their node
				Unit *u = getUnitByName((*nodeIter)->getName());
				if (u != NULL) return u;
	         }
		}
	}
	return NULL;
}

osg::Vec3 HTApplication::terrainHitTest(float x, float y) {
	osg::Vec3 pos;

	osgUtil::LineSegmentIntersector* picker;
    // remap the mouse x,y into viewport coordinates.
    osg::Viewport* viewport = GetCamera()->GetOSGCamera()->getViewport();
    float mx = viewport->x() + (int)((float)viewport->width()*(x*0.5f+0.5f));
    float my = viewport->y() + (int)((float)viewport->height()*(y*0.5f+0.5f));
    picker = new osgUtil::LineSegmentIntersector( osgUtil::Intersector::WINDOW, mx, my );
    osgUtil::IntersectionVisitor iv(picker);

    GetCamera()->GetOSGCamera()->accept(iv);

    if (picker->containsIntersections())
    {
    	osgUtil::LineSegmentIntersector::Intersections& intersections = picker->getIntersections();

		for (osgUtil::LineSegmentIntersector::Intersections::iterator it=intersections.begin(); it!=intersections.end(); ++it) {
			osg::NodePath nodePath = it->nodePath;
	         for(  osg::NodePath::iterator nodeIter = nodePath.begin(); nodeIter != nodePath.end(); ++nodeIter ) {
	        	 if ( (*nodeIter)->getName().length() >= m_sWorldName.length() ) {
					if ( (*nodeIter)->getName().substr(0,m_sWorldName.length()) == m_sWorldName ) {
						pos = it->getWorldIntersectPoint();
					}
				}
	         }
		}
    }
    return pos;
}

void HTApplication::moveCameraToLookAt(osg::Vec3 pos) {
	pos = mapPointToTerrain(pos);

	dtCore::Transform transform;
	osg::Matrix mat;
	osg::Quat q;

	//get current camera orientation
	m_MotionModel->GetTarget()->GetTransform(transform);
	transform.GetRotation(mat);
	mat.get(q);
	osg::Vec3 direction = q*osg::Vec3(0,1,0);

	osg::Vec3 vector = direction * m_MotionModel->GetDistance();

	dtCore::Transform tx;

#if DELTA3D_VER >= 23
        tx.Set(pos[0]-vector[0],pos[1]-vector[1],pos[2]-vector[2], pos[0],pos[1],pos[2], 0,0,1);
#else
        tx.SetLookAt(pos[0]-vector[0],pos[1]-vector[1],pos[2]-vector[2], pos[0],pos[1],pos[2], 0,0,1);
#endif
	theApp->GetCamera()->SetTransform(tx);
}

void HTApplication::setSelectedUnit(Unit *unit) {
	m_SelectedUnits.clear();
	//deselect all tanks
	for (unit_it it = m_Units.begin(); it != m_Units.end(); it++)
	{
		(*it)->setSelected(false);
	}
	//select current tank
	if (unit->getIsControlled() ) unit->setSelected(true);
	m_SelectedUnits.push_back(unit);
}

void HTApplication::FocusOnUnit(const std::string & name)
{
	for (unit_it it = m_Units.begin(); it != m_Units.end(); it++)
	{
		if ( (*it)->GetName() == name)
		{
			//move camera to position
			setLookAtUnit(it->get());
			//moveCameraToLookAt( (*it)->getPosition() );
			setSelectedUnit((*it).get());
			break;
		}
	}
}

void HTApplication::setLookAtUnit(Unit* unit) {
	mCameraLookAtUnit = unit;
}

void HTApplication::BroadcastEvent(Event::EventType e, Unit *unit1, Unit *unit2, osg::Vec3 pos) {
	HTAppBase::BroadcastEvent(e,unit1,unit2,pos);

	//transmit to other participants
	std::vector<std::string> participants;
	std::vector<double> data;
	switch (e) {
		case Event::POSITION:
		case Event::HEADING:
		case Event::POSITION_AND_HEADING:
		case Event::NEW_GOAL:
		case Event::SELECTED:
		case Event::DETONATION:
		case Event::SEES:
		case Event::HIDE:
		case Event::FIRE:
		case Event::HURT:
		case Event::PAUSE:
		case Event::GAME_OVER:
		case Event::TIME_SYNC:
			//nothing
			return;
		case Event::TIME_SCALE:
			//this player
			if (m_MySide == RED) {
				participants.push_back("RedPlayer ");
			} else if (m_MySide == BLUE) {
				participants.push_back("BluePlayer");
			}
			//data
			data.push_back(mDesiredTimeScale);
			break;
	}
	EventPacket packet( e, participants, data );
	m_Net->SendEvent( packet );
	//log
	EventLog::GetInstance().log(e, participants, data);
}

void HTApplication::ReceiveEvent(Event::EventType event, std::vector<std::string>& participants, std::vector<double>& data) {
	HTAppBase::ReceiveEvent(event, participants, data);

	Projectile *projectile;
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
			//show explosion at pos
			if (participants.at(0).find("Soldier") != std::string::npos) {
				projectile = new Projectile(0.5,1,1000,Projectile::BULLET);
			} else {
				projectile = new Projectile(0.5,1,1000,Projectile::SHELL);
			}
			projectile->init();
			projectile->setPosition(osg::Vec3(data.at(0), data.at(1), data.at(2)));
			projectile->detonate();
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
			//nothing
			break;
		case Event::PAUSE:
			//pause sim TODO replace this with m_InstanceID
			if (data[0] == RED) {
				hudGUI->ShowPausedRed(data[1]);
         	}
         	if (data[0] == BLUE) {
         		hudGUI->ShowPausedBlue(data[1]);
         	}
			break;
        case Event::GAME_OVER:
			// do not do anything
			break;
        case Event::TIME_SYNC:
        	//set our remaining time and time scale to that of the server
        	mRemainingTime = data[0];
        	setTimeScale(data[1]);
        	hudGUI->SetTimeScale(getTimeScale(),mDesiredTimeScale,mRequestedTimeScale);
        	break;
        case Event::TIME_SCALE:
        	//save request
        	mRequestedTimeScale = data[0];
        	//if this request is what we want or less than what we want then use it
        	if (mRequestedTimeScale <= mDesiredTimeScale) {
        		//if we're slowing down then assume that we want it too
        		//this makes sure both players explicitly increment the scale after it has been slowed
        		if (mRequestedTimeScale < getTimeScale()) {
        			mDesiredTimeScale = mRequestedTimeScale;
        		}
        		setTimeScale(mRequestedTimeScale);
        		//send sync in reply (sets the scale on all recipients)
        		BroadcastEvent(Event::TIME_SYNC);
        	}
        	hudGUI->SetTimeScale(getTimeScale(),mDesiredTimeScale,mRequestedTimeScale);
        	break;

        default:
        	LOG_DEBUG("Unhandled event type");
			return;
	}
}

void HTApplication::displayUnitHealth(Unit *unit, float health) {
	hudGUI->setHealth(unit->getHealthWindow(), health);
}

void HTApplication::displayUnitReload(UnitFiring *unit, double reload) {
	hudGUI->setReload(unit->getReloadWindow(), reload);
}

void HTApplication::zoomUpdate(float dist) {
	for (unit_it it = m_Units.begin(); it != m_Units.end(); it++)
	{
		float alpha = dist/1000;
		if (alpha > 1) alpha = 1;
		if (alpha < 0.5) alpha = 0.5;
		(*it)->setUnitMarkerAlpha(alpha);
	}
}

void HTApplication::setSelectedFormation(std::string formation) {
	//remove existing inverse models
	mInverseModels.clear();

	//create a separate vector of units that are selected, and pass to setFormation
	std::vector<Unit*> units;
	for (unsigned int i=0; i < m_SelectedUnits.size(); i++) {
		units.push_back(m_SelectedUnits.at(i).get());
	}

	InverseModelFormation::UnitFormation form;
	if (formation == "Circle") {
		form = InverseModelFormation::CIRCLE;
	} else if (formation == "Wedge") {
		form = InverseModelFormation::WEDGE;
	} else if (formation == "Line") {
		form = InverseModelFormation::LINE;
	} else if (formation == "Column") {
		form = InverseModelFormation::COLUMN;
	} else {
		form = InverseModelFormation::NONE;
	}

	InverseModel *group = NULL;
	if (form != InverseModelFormation::NONE) {
		group = new InverseModelFormation(units, form);
	} else {
		if (formation == "Attack") {
			group = new InverseModelAttack(units);
		} else if (formation == "Retreat") {
			group = new InverseModelRetreat(units);
		} else if (formation == "Defend") {
			group = new InverseModelDefend(units);
		} else if (formation == "Pincer") {
			group = new InverseModelPincer(units);
		} else if (formation == "Convoy") {
			group = new InverseModelConvoy(units);
		}
	}

	//add to list of inverse models
	if (group) addInverseModel(group);
}

void HTApplication::clearScene() {
	HTAppBase::clearScene();
	//remove gui
	hudGUI->removeUnitInfoDisplays();
}

bool HTApplication::isHeadless() {
	//overrides HTAppBase::isHeadless()
	return false;
}

void HTApplication::requestMultiplyTimeScale(double scale) {
	mDesiredTimeScale *= scale;
	BroadcastEvent(Event::TIME_SCALE);
	if (mScenarioData->numInstances == 1) {
		//dont wait for a reply if we're the only instance
		multiplyTimeScale(scale);
		hudGUI->SetTimeScale(getTimeScale(),-1,-1);
	} else {
		if (mDesiredTimeScale < getTimeScale()) {
			//assume other player always agrees to scale decreases
			mRequestedTimeScale = mDesiredTimeScale;
		}
		hudGUI->SetTimeScale(getTimeScale(),mDesiredTimeScale,mRequestedTimeScale);
	}
}

void HTApplication::EndGame( UnitSide winner ) {
	HTAppBase::EndGame(winner);

	if (winner == BLUE) {
		hudGUI->ShowGameOverMessage("BLUE WINS");
	} else {
		hudGUI->ShowGameOverMessage("RED WINS");
	}
}

void HTApplication::EndGameGUI() {
	// pause the game to prevent further logging
	dtCore::System::GetInstance().SetPause( 1);
}
