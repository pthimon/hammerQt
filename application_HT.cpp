/* -*-c++-*-
 * Delta3D Open Source Game and Simulation Engine
 * Copyright (C) 2004-2005 MOVES Institute
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * John K. Grant
 */

#include "common.h"

#pragma GCC diagnostic ignored "-Wextra"

#include <osgViewer/CompositeViewer>

#include <dtABC/applicationconfigwriter.h>
#include <dtABC/applicationconfighandler.h>
#include <dtABC/applicationconfigdata.h>           // for return type, member
#include <dtCore/stats.h>

#include <dtCore/system.h>
#include <dtCore/view.h>
#include <dtCore/databasepager.h>
#include <dtCore/camera.h>
#include <dtCore/scene.h>
#include <dtCore/globals.h>
#include <dtCore/deltawin.h>
#include <dtUtil/log.h>
#include <dtUtil/stringutils.h>
#include <dtUtil/xercesparser.h>
#include <dtUtil/librarysharingmanager.h>
#include <dtCore/mouse.h>

#include <cassert>

#include <dtUtil/xercesutils.h>
#include <xercesc/util/XMLString.hpp>

#include <osgViewer/Viewer>
#include <osg/io_utils>
#include <osg/Version>

#include "application_HT.h"

#pragma GCC diagnostic warning "-Wextra"

using namespace dtABC;
XERCES_CPP_NAMESPACE_USE

IMPLEMENT_MANAGEMENT_LAYER(Application_HT)

const std::string Application_HT::SIM_FRAME_RATE("System.SimFrameRate");
const std::string Application_HT::MAX_TIME_BETWEEN_DRAWS("System.MaxTimeBetweenDraws");
const std::string Application_HT::USE_FIXED_TIME_STEP("System.UseFixedTimeStep");

///////////////////////////////////////////////////////////////////////////////
Application_HT::Application_HT( const std::string& configFilename,
		const std::string& hostName, int port, bool wait_for_client, const std::string& scenarioFilename, int id, std::string& replayFile, std::string& configFile, dtCore::DeltaWin* win)
	: HTAppBase(configFilename, hostName, port, wait_for_client, scenarioFilename, id, replayFile, configFile),
	  mKeyboardListener(new dtCore::GenericKeyboardListener()),
	  mMouseListener(new dtCore::GenericMouseListener())
{
   RegisterInstance(this);

   mKeyboardListener->SetPressedCallback (dtCore::GenericKeyboardListener::CallbackType     (this, &Application_HT::KeyPressed));
   mKeyboardListener->SetReleasedCallback(dtCore::GenericKeyboardListener::CallbackType     (this, &Application_HT::KeyReleased));
   mMouseListener->SetPressedCallback    (dtCore::GenericMouseListener::ButtonCallbackType  (this, &Application_HT::MouseButtonPressed));
   mMouseListener->SetReleasedCallback   (dtCore::GenericMouseListener::ButtonCallbackType  (this, &Application_HT::MouseButtonReleased));
   mMouseListener->SetClickedCallback    (dtCore::GenericMouseListener::ClickCallbackType   (this, &Application_HT::MouseButtonDoubleClicked));
   mMouseListener->SetMovedCallback      (dtCore::GenericMouseListener::MovementCallbackType(this, &Application_HT::MouseMoved));
   mMouseListener->SetDraggedCallback    (dtCore::GenericMouseListener::MovementCallbackType(this, &Application_HT::MouseDragged));
   mMouseListener->SetScrolledCallback   (dtCore::GenericMouseListener::WheelCallbackType   (this, &Application_HT::MouseScrolled));

   mWindow = win;

   CreateInstances(); //create default Viewer View

   mStats = new dtCore::StatsHandler(*mCompositeViewer);

   if (!configFilename.empty())
   {
      std::string foundPath = dtCore::FindFileInPathList(configFilename);
      if (foundPath.empty())
      {
         LOG_WARNING("Application: Can't find config file, " + configFilename + ", using defaults instead.")
      }
      else if (!ParseConfigFile(foundPath))
      {
         LOG_WARNING("Application: Error loading config file, using defaults instead.");
      }
   }
}

///////////////////////////////////////////////////////////////////////////////
void Application_HT::Config()
{
   BaseClass::Config();

   //Temporary insurance policy.  If an application is expecting an openGL
   //context outside of the draw, setting the following environment variable will
   //keep a context valid throughout the whole frame.  This is a bit of a crutch
   //for applications upgrading to OSG 2.6.0 that are crashing due to openGL
   //context issues.  Users should not rely on this.
#if defined(OPENSCENEGRAPH_MAJOR_VERSION) && OPENSCENEGRAPH_MAJOR_VERSION >= 2 && defined(OPENSCENEGRAPH_MINOR_VERSION) && OPENSCENEGRAPH_MINOR_VERSION >= 6
   char* deltaReleaseContext = getenv("DELTA_RELEASE_CONTEXT");
   if (deltaReleaseContext)
   {
      GetCompositeViewer()->setReleaseContextAtEndOfFrameHint(false);
   }
#endif

   ReadSystemProperties();
}

///////////////////////////////////////////////////////////////////////////////
void Application_HT::ReadSystemProperties()
{
   std::string value;

   value = GetConfigPropertyValue(SIM_FRAME_RATE);

   if (!value.empty())
   {
      double simFrameRate = dtUtil::ToDouble(value);
      dtCore::System::GetInstance().SetFrameRate(simFrameRate);
   }

   value.clear();

   value = GetConfigPropertyValue(MAX_TIME_BETWEEN_DRAWS);
   if (!value.empty())
   {
      double timeBetween = dtUtil::ToDouble(value);
      dtCore::System::GetInstance().SetMaxTimeBetweenDraws(timeBetween);
   }

   value.clear();

   value = GetConfigPropertyValue(USE_FIXED_TIME_STEP);
   if (!value.empty())
   {
      bool useFixed = dtUtil::ToType<bool>(value);
      dtCore::System::GetInstance().SetUseFixedTimeStep(useFixed);
   }

   if (GetView()->GetDatabasePager())
   {
      GetView()->GetDatabasePager()->SetConfiguration(this);
   }
}

///////////////////////////////////////////////////////////////////////////////
Application_HT::~Application_HT()
{
   mCompositeViewer = NULL;
   delete mStats;
   DeregisterInstance(this);
}

///////////////////////////////////////////////////////////////////////////////
void Application_HT::Run()
{
   dtCore::System::GetInstance().Run();
}

///////////////////////////////////////////////////////////////////////////////
void Application_HT::PreFrame(const double deltaSimTime)
{
	BaseClass::PreFrame(deltaSimTime);
}

///////////////////////////////////////////////////////////////////////////////
void Application_HT::Frame(const double deltaSimTime)
{
   // NOTE: The new version OSG (2.2) relies on absolute frame time
   // to update drawables; especially particle systems.
   // The time delta will be ignored here and the absolute simulation
   // time passed to the OSG scene updater.
   mCompositeViewer->frame(dtCore::System::GetInstance().GetSimTimeSinceStartup());
}

///////////////////////////////////////////////////////////////////////////////
void Application_HT::PostFrame(const double deltaSimTime)
{
	BaseClass::PostFrame(deltaSimTime);
}

///////////////////////////////////////////////////////////////////////////////
bool Application_HT::KeyPressed(const dtCore::Keyboard* keyboard, int kc)
{
   switch (kc)
   {
   case osgGA::GUIEventAdapter::KEY_Escape:
      {
         Quit();
         return true;
      } break;

   default:
      {
         return false;
      } break;
   }

   return false;
}

///////////////////////////////////////////////////////////////////////////////
bool Application_HT::KeyReleased(const dtCore::Keyboard* keyboard, int kc)
{
   return false;
}

//////////////////////////////////////////////////////////////////////////
bool Application_HT::MouseButtonPressed(const dtCore::Mouse *mouse, dtCore::Mouse::MouseButton button)
{
   return false;
}

//////////////////////////////////////////////////////////////////////////
bool Application_HT::MouseButtonReleased(const dtCore::Mouse *mouse, dtCore::Mouse::MouseButton button)
{
   return false;
}

//////////////////////////////////////////////////////////////////////////
bool Application_HT::MouseButtonDoubleClicked(const dtCore::Mouse *mouse, dtCore::Mouse::MouseButton button, int clickCount)
{
   return false;
}

//////////////////////////////////////////////////////////////////////////
bool Application_HT::MouseMoved(const dtCore::Mouse* mouse, float x, float y)
{
   return false;
}

//////////////////////////////////////////////////////////////////////////
bool Application_HT::MouseDragged(const dtCore::Mouse* mouse, float x, float y)
{
   return false;
}

//////////////////////////////////////////////////////////////////////////
bool Application_HT::MouseScrolled(const dtCore::Mouse* mouse, int delta)
{
   return false;
}

///////////////////////////////////////////////////////////////////////////////
void Application_HT::CreateInstances(const std::string& name, int x, int y, int width,
                                  int height, bool cursor, bool fullScreen)
{
   //create the instances and hook-up the default
   //connections.  The instance attributes may be
   //overwritten using a config file
   BaseClass::CreateInstances();

   if (mWindow == NULL)
   {
       mWindow = new dtCore::DeltaWin(name, x, y, width, height, cursor, fullScreen);
   }

   GetCamera()->SetWindow(mWindow.get());

   mCompositeViewer = new osgViewer::CompositeViewer;
//   mCompositeViewer->setThreadingModel(osgViewer::CompositeViewer::SingleThreaded);
   mCompositeViewer->addView(mViewList.front()->GetOsgViewerView());

   GetKeyboard()->AddKeyboardListener(mKeyboardListener.get());
   GetMouse()->AddMouseListener(mMouseListener.get());
}

///////////////////////////////////////////////////////////////////////////////
const std::string& Application_HT::GetConfigPropertyValue(
         const std::string& name, const std::string& defaultValue) const
{
   AppConfigPropertyMap::const_iterator i = mConfigProperties.find(name);
   if (i == mConfigProperties.end())
   {
      return defaultValue;
   }
   else
   {
      return i->second;
   }
}

///////////////////////////////////////////////////////////////////////////////
void Application_HT::SetConfigPropertyValue(const std::string& name, const std::string& value)
{
   if (!mConfigProperties.insert(std::make_pair(name, value)).second)
   {
      AppConfigPropertyMap::iterator i = mConfigProperties.find(name);
      /// "i" can't be the "end()" because the insert returned false, meaning it does have that key.
      i->second = value;
   }
}

///////////////////////////////////////////////////////////////////////////////
void Application_HT::RemoveConfigPropertyValue(const std::string& name)
{
   mConfigProperties.erase(name);
}

///////////////////////////////////////////////////////////////////////////////
bool Application_HT::ParseConfigFile(const std::string& file)
{
   ApplicationConfigHandler handler;
   dtUtil::XercesParser parser;

   bool parsed_well = parser.Parse(file, handler, "application.xsd");
   if (!parsed_well)
   {
      LOG_ERROR("The Application config file, " + file + ", wasn't parsed correctly.");
      return(false);
   }

   AppXMLApplicator applicator;
   bool applied_well = applicator(handler.mConfigData, this);
   if (!applied_well)
   {
      LOG_ERROR("The Application config file data wasn't applied correctly.");
      return(false);
   }

   return true;
}

///////////////////////////////////////////////////////////////////////////////
std::string Application_HT::GenerateDefaultConfigFile(const std::string& filename)
{
   std::string existingfile = dtCore::FindFileInPathList(filename);

   if (!existingfile.empty())
   {
      LOG_WARNING("Can't generate new configuration file: file already exists: " + existingfile);
      return existingfile;
   }

   // write out a new file
   ApplicationConfigWriter writer;
   writer(filename, GetDefaultConfigData());

   // return the resource path to the new file
   return dtCore::FindFileInPathList(filename);
}


///////////////////////////////////////////////////////////////////////////////
void Application_HT::SetNextStatisticsType()
{
   mStats->SelectNextType();
}


///////////////////////////////////////////////////////////////////////////////
dtABC::ApplicationConfigData Application_HT::GetDefaultConfigData()
{
   ApplicationConfigData data;

   data.WINDOW_X = 100;
   data.WINDOW_Y = 100;

   data.SHOW_CURSOR = true;
   data.FULL_SCREEN = false;
   data.CHANGE_RESOLUTION = false;

   data.CAMERA_NAME = "defaultCam";
   data.SCENE_NAME = "defaultScene";
   data.WINDOW_NAME = "defaultWin";
   data.VIEW_NAME = "defaultView";

   data.SCENE_INSTANCE = "defaultScene";
   data.WINDOW_INSTANCE = "defaultWin";
   data.CAMERA_INSTANCE = "defaultCam";

   data.RESOLUTION.width = 640;
   data.RESOLUTION.height = 480;
   data.RESOLUTION.bitDepth = 24;
   data.RESOLUTION.refresh = 60;

   dtUtil::Log& logger = dtUtil::Log::GetInstance();
   data.LOG_LEVELS.insert(make_pair(logger.GetName(), logger.GetLogLevelString(dtUtil::Log::LOG_WARNING)));

   return data;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// --- applicator's implementation --- //
bool Application_HT::AppXMLApplicator::operator ()(const dtABC::ApplicationConfigData& data, Application_HT* app)
{
   // set up the View
   if (!data.VIEW_NAME.empty())
   {
      dtCore::View* view = app->GetView();
      if (view != NULL)
      {
         view->SetName(data.VIEW_NAME);
      }
   }

   // set up the scene
   if (!data.SCENE_NAME.empty())
   {
      dtCore::Scene* scene = app->GetScene();
      if (scene != NULL)
      {
         scene->SetName(data.SCENE_NAME);
      }
   }

   // set up the camera
   if (!data.CAMERA_NAME.empty())
   {
      dtCore::Camera* camera = app->GetCamera();
      if (camera != NULL)
      {
         camera->SetName(data.CAMERA_NAME);
      }
   }

   // apply the window settings
   dtCore::DeltaWin* dwin = app->GetWindow();

   for (std::map<std::string, std::string>::const_iterator i = data.LOG_LEVELS.begin();
      i != data.LOG_LEVELS.end(); ++i)
   {
      dtUtil::Log& logger = dtUtil::Log::GetInstance(i->first);

      logger.SetLogLevel(logger.GetLogLevelForString(i->second));
   }

   dtUtil::LibrarySharingManager& lsm = dtUtil::LibrarySharingManager::GetInstance();
   for (std::vector<std::string>::const_iterator i = data.LIBRARY_PATHS.begin();
      i != data.LIBRARY_PATHS.end(); ++i)
   {
      lsm.AddToSearchPath(*i);
   }

   for (std::map<std::string, std::string>::const_iterator i = data.mProperties.begin();
      i != data.mProperties.end(); ++i)
   {
      app->SetConfigPropertyValue(i->first, i->second);
   }

   // John's unwittingly caught a confusing aspect of the Applications's config file here.
   // Historically, the Window's "name" attribute is used for the WindowTitle, while other
   // elements, such as Screen, use the "name" attribute for the Base name. John had followed
   // convention and just called SetName. Perhaps we need to add a new paramter called
   // "title". However, this would break the expectation of users with previously written
   // configuration files. Maybe we could do an automatically update whenver a config file
   // without the "title" attribute is passed to Application. See Case 722 -osb
   dwin->SetWindowTitle(data.WINDOW_NAME);
   dwin->SetName(data.WINDOW_NAME); // Perhaps a different parameter is needed for this?

   dwin->SetPosition(data.WINDOW_X, data.WINDOW_Y, data.RESOLUTION.width, data.RESOLUTION.height);
   dwin->ShowCursor(data.SHOW_CURSOR);
   dwin->SetFullScreenMode(data.FULL_SCREEN);

   // change the resolution if needed and valid
   if (data.CHANGE_RESOLUTION)
   {
      if (dwin->IsValidResolution(data.RESOLUTION))
      {
         dwin->ChangeScreenResolution(data.RESOLUTION);
      }
   }

   bool valid = true; //optimistic

   // connect the camera, scene, and window
   // since they might not be the same as the app's instances, we will use the instance management layer
   dtCore::DeltaWin* win = dtCore::DeltaWin::GetInstance(data.WINDOW_INSTANCE);
   dtCore::Camera* camera = dtCore::Camera::GetInstance(data.CAMERA_INSTANCE);
   dtCore::View* view = dtCore::View::GetInstance(data.VIEW_NAME);
   dtCore::Scene* sinst = dtCore::Scene::GetInstance(data.SCENE_INSTANCE);

   if ((win != NULL) && (camera != NULL))
   {
      camera->SetWindow(win);
   }
   else
   {
      LOG_WARNING("Application:Can't find instance of DeltaWin, " + data.WINDOW_INSTANCE );
      valid = false;
   }

   if ((camera != NULL) && (view != NULL))
   {
      view->SetCamera(camera);
   }
   else
   {
       LOG_WARNING("Application:Can't find instance of Camera, " + data.CAMERA_INSTANCE );
       valid = false;
   }

   if ((sinst != NULL) && (view != NULL))
   {
      view->SetScene(sinst);
   }
   else
   {
      LOG_WARNING("Application:Can't find instance of Scene, " + data.SCENE_INSTANCE );
      valid = false;
   }

   return valid;
}
////////////////////////////////////////////////////////
void Application_HT::AddView(dtCore::View &view)
{
   if (mCompositeViewer.get() == NULL)
   {
      mCompositeViewer = new osgViewer::CompositeViewer;
   }

   mCompositeViewer->addView(view.GetOsgViewerView());
   mViewList.push_back(&view);
}

////////////////////////////////////////////////////////
void Application_HT::RemoveView(dtCore::View &view)
{
   ViewList::iterator it = std::find(mViewList.begin(), mViewList.end(), &view);
   if (it != mViewList.end())
   {
      mViewList.erase(it);
      mCompositeViewer->removeView(view.GetOsgViewerView());
   }
}
