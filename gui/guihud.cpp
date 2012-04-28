/*
* Delta3D Open Source Game and Simulation Engine
* Copyright (C) 2005, BMH Associates, Inc.
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
* @author Curtiss Murphy
*/

#include "common.h"
#include "guihud.h"

#include <dtCore/object.h>
#include <dtCore/globals.h>
#include <dtCore/flymotionmodel.h>
#include <dtCore/deltawin.h>
#include <dtUtil/exception.h>

#include <dtGame/binarylogstream.h>
#include <dtGame/logtag.h>
#include <dtGame/logkeyframe.h>
#include <dtGame/logstatus.h>
#include <dtGame/defaultmessageprocessor.h>
#include <dtGame/basemessages.h>
#include <dtGame/messagetype.h>
#include <dtGame/gamemanager.h>
#include <dtGame/logcontroller.h>
#include <dtGame/logstatus.h>
#include <dtGame/serverloggercomponent.h>
#include <dtGame/taskcomponent.h>
#include <dtActors/taskactor.h>

//#include <dtDAL/enginepropertytypes.h>
//#include <dtDAL/project.h>
//#include <dtDAL/map.h>
//#include <dtDAL/actorproxy.h>
//#include <dtDAL/transformableactorproxy.h>

#include <dtGUI/resourceprovider.h>

#include <GL/gl.h>
#include <GL/glu.h>

#include <ctime>
#include <sstream>
#include <iomanip>

#include "../HTapp.h"
#include "../units/UnitFiring.h"

// TODO: replace this with a platform-independant wrapper
#if defined (WIN32) || defined (_WIN32) || defined (__WIN32__)
   #ifndef snprintf
      #define snprintf _snprintf
   #endif // snprintf
#endif // WIN32

const double SIZE_OF_MMAP = 150;
const double FRAME_OF_MMAP = 5;

//////////////////////////////////////////////////////////////////////////
GUIHUD::GUIHUD(HTApplication *app, dtCore::DeltaWin *win, std::string map_name)
 : mWin(win),
   mMainWindow(NULL),
   mGUI(NULL),
   mApp(app),
   m_NumUnitDisplays(0)
{
   SetupGUI(win, map_name);
}

//////////////////////////////////////////////////////////////////////////
GUIHUD::~GUIHUD()
{
   // mGUI->ShutdownGUI();
}

//////////////////////////////////////////////////////////////////////////
void GUIHUD::SetupGUI(dtCore::DeltaWin *win, std::string map_name)
{
	try {
		// Initialize CEGUI
		mGUI = new dtGUI::CEUIDrawable(win, theApp->GetKeyboard(), theApp->GetMouse());

		CEGUI::Logger::getSingleton().setLoggingLevel(CEGUI::Insane);

		CEGUI::SchemeManager::getSingleton().loadScheme("WindowsLook.scheme");

		mWm = CEGUI::WindowManager::getSingletonPtr();
		ISm = CEGUI::ImagesetManager::getSingletonPtr();

		// mFont = CEGUI::FontManager::getSingleton().createFont("fkp-16.font");
		mFont = CEGUI::FontManager::getSingleton().createFont("DejaVuSans-10.font");
		m_HugeFont = CEGUI::FontManager::getSingleton().createFont("DejaVuSans-30.font");
		mMainWindow = mWm->createWindow("DefaultWindow", "root");
		CEGUI::System::getSingleton().setGUISheet(mMainWindow);

      	// to look more like a real application, we override the autoscale setting
	    // for both skin and font
	    CEGUI::Imageset* wndlook = ISm->getImageset("WindowsLook");
	    wndlook->setAutoScalingEnabled(false);
		mFont->setProperty("AutoScaled", "false");

		//Semi-transparent window for the background to the bottom toolbar
		//(makes all children components also semi-transparent with no way of changing their alpha individually!)
		toolWndBg = mWm->createWindow("WindowsLook/Static", "toolWindowBg");
		mMainWindow->addChildWindow(toolWndBg);
		toolWndBg->setVerticalAlignment(CEGUI::VA_BOTTOM);
		toolWndBg->setPosition(CEGUI::UVector2(CEGUI::UDim(0,0), CEGUI::UDim(0,0)));
		toolWndBg->setSize(CEGUI::UVector2(CEGUI::UDim(1,-160), CEGUI::UDim(0,70)));
		toolWndBg->setProperty("FrameEnabled", "false");
		toolWndBg->setAlpha(0.5f);

		//window to contain the bottom toolbar components
		toolWnd = mWm->createWindow("DefaultWindow", "toolWindow");
		mMainWindow->addChildWindow(toolWnd);
		toolWnd->setVerticalAlignment(CEGUI::VA_BOTTOM);
		toolWnd->setPosition(CEGUI::UVector2(CEGUI::UDim(0,0), CEGUI::UDim(0,0)));
		toolWnd->setSize(CEGUI::UVector2(CEGUI::UDim(1,-160), CEGUI::UDim(0,70)));

		//window to contain the time
		m_Time = mWm->createWindow("WindowsLook/StaticText", "timeWindow");
		mMainWindow->addChildWindow(m_Time);
		m_Time->setPosition(CEGUI::UVector2(CEGUI::UDim(1,-150), CEGUI::UDim(1,-SIZE_OF_MMAP-80)));
		m_Time->setSize(CEGUI::UVector2(CEGUI::UDim(0,150), CEGUI::UDim(0,120)));
		m_Time->setProperty("FrameEnabled", "false");
		m_Time->setProperty("BackgroundEnabled", "false");
		SetTime(0,0);

		//window to contain current time scale
		m_TimeScale = mWm->createWindow("WindowsLook/StaticText", "timeScaleWindow");
		mMainWindow->addChildWindow(m_TimeScale);
		m_TimeScale->setPosition(CEGUI::UVector2(CEGUI::UDim(1,-150), CEGUI::UDim(0,20)));
		m_TimeScale->setSize(CEGUI::UVector2(CEGUI::UDim(0,140), CEGUI::UDim(0,mFont->getLineSpacing(1.0f)*3)));
		m_TimeScale->setProperty("FrameEnabled", "false");
		m_TimeScale->setProperty("BackgroundEnabled", "false");
		m_TimeScale->setProperty("HorzFormatting", "RightAligned");
		m_TimeScale->setProperty("VertFormatting", "TopAligned");
		SetTimeScale(1,1,1);

		//Map window sem-transparent background
		//CEGUI::FrameWindow* fWnd = static_cast<CEGUI::FrameWindow*>( mWm->createWindow("WindowsLook/FrameWindow", "mapWindow"));
		mapWnd = mWm->createWindow("WindowsLook/Static", "mapWindow");
		mMainWindow->addChildWindow(mapWnd);
		mapWnd->setHorizontalAlignment(CEGUI::HA_RIGHT);
		mapWnd->setVerticalAlignment(CEGUI::VA_BOTTOM);
		mapWnd->setPosition(CEGUI::UVector2(CEGUI::UDim(0,0), CEGUI::UDim(0,0)));
		mapWnd->setSize(CEGUI::UVector2(CEGUI::UDim(0,SIZE_OF_MMAP+2*FRAME_OF_MMAP), CEGUI::UDim(0,SIZE_OF_MMAP+2*FRAME_OF_MMAP)));
		mapWnd->setProperty("FrameEnabled", "false");
		mapWnd->setAlpha(0.5f);
		mapWnd->setRiseOnClickEnabled(false);

		//Map:  load an image to use as a background
	    ISm->createImagesetFromImageFile("MapImage", map_name);

	    map = mWm->createWindow("WindowsLook/StaticImage", "mapWindow/map");
	    mMainWindow->addChildWindow(map);
	    map->setHorizontalAlignment(CEGUI::HA_RIGHT);
		map->setVerticalAlignment(CEGUI::VA_BOTTOM);
		map->setPosition(CEGUI::UVector2(CEGUI::UDim(0,-FRAME_OF_MMAP), CEGUI::UDim(0,-FRAME_OF_MMAP)));
		map->setSize(CEGUI::UVector2(CEGUI::UDim(0,SIZE_OF_MMAP), CEGUI::UDim(0,SIZE_OF_MMAP)));
	    // disable frame and standard background
	    map->setProperty("FrameEnabled", "false");
	    map->setProperty("BackgroundEnabled", "false");
	    // set the background image
	    map->setProperty("Image", "set:MapImage image:full_image");
		map->setAlpha(0.7f);
		map->setRiseOnClickEnabled(false);

		//Some test text
		st = mWm->createWindow("WindowsLook/StaticText", "TextWindow");
		toolWnd->addChildWindow(st);
		st->setPosition(CEGUI::UVector2(cegui_reldim(0.0f), cegui_reldim( 0.0f)));
		st->setSize(CEGUI::UVector2(cegui_reldim(1.0f), cegui_reldim( 1.0f)));
		st->setText("Test");
		// disable frame and background on static control
		st->setProperty("FrameEnabled", "false");
		st->setProperty("BackgroundEnabled", "false");

		// red pause text
		m_PausedRed = mWm->createWindow("WindowsLook/StaticText", "RedPaused");
		mMainWindow->addChildWindow(m_PausedRed);
		std::string message = "Paused by Red Player";
		m_PausedRed->setText(message);
		m_PausedRed->setSize(CEGUI::UVector2(CEGUI::UDim(0, mFont->getTextExtent(message) ), CEGUI::UDim(0,mFont->getLineSpacing(1.0f))));
		m_PausedRed->setPosition( CEGUI::UVector2(CEGUI::UDim(0.5f,-mFont->getTextExtent(message)/2), cegui_reldim(0.2f) ) );
		// disable frame and background on static control
		m_PausedRed->setProperty("FrameEnabled", "false");
		m_PausedRed->setProperty("BackgroundEnabled", "false");
		m_PausedRed->setVisible(false);

		// blue pause text
		m_PausedBlue = mWm->createWindow("WindowsLook/StaticText", "BluePaused");
		mMainWindow->addChildWindow(m_PausedBlue);
		message = "Paused by Blue Player";
		m_PausedBlue->setText(message);
		m_PausedBlue->setSize(CEGUI::UVector2(CEGUI::UDim(0, mFont->getTextExtent(message) ), CEGUI::UDim(0,mFont->getLineSpacing(1.0f))));
		m_PausedBlue->setPosition( CEGUI::UVector2(CEGUI::UDim(0.5f,-mFont->getTextExtent(message)/2), CEGUI::UDim(0.2f, mFont->getLineSpacing(2.0f)) ) );
		// disable frame and background on static control
		m_PausedBlue->setProperty("FrameEnabled", "false");
		m_PausedBlue->setProperty("BackgroundEnabled", "false");
		m_PausedBlue->setVisible(false);

		// create a menubar.
		// this will fit in the top of the screen and have options for the simulator
	    CEGUI::Window* bar = mWm->createWindow("WindowsLook/Menubar");
	    bar->setArea(CEGUI::UDim(0,0),CEGUI::UDim(0,0),CEGUI::UDim(1,0),CEGUI::UDim(0,mFont->getLineSpacing(1.2f)));
	    bar->setAlwaysOnTop(true); // we want the menu on top
	    mMainWindow->addChildWindow(bar);

	    // fill out the menubar
	    createMenu(bar);

	    // compatibility for Balint
	    CEGUI::Window* sheet = CEGUI::System::getSingleton().getGUISheet();
        if(sheet) sheet->setAlpha(0.99);

        // add the compass
        m_Compass = new dtCore::Compass(theApp->GetCamera());
  		m_Compass.get()->SetScreenPosition(100,100);
  		mGUI->AddChild(m_Compass.get());

    	// add the triangle marking the camera position on the minimap
  		m_CamTriangle = new CamTriangle(theApp->GetCamera());
  		m_CamTriangle.get()->SetMinimapArea(FRAME_OF_MMAP, FRAME_OF_MMAP, SIZE_OF_MMAP, SIZE_OF_MMAP);

  		mGUI->AddChild(m_CamTriangle.get());
	}
	catch(CEGUI::Exception &e)
	{
		std::ostringstream oss;
		oss << "CEGUI while setting up GUI: " << e.getMessage().c_str();
	}

}

/*************************************************************************
    Creates the menu bar and fills it up :)
*************************************************************************/
void GUIHUD::createMenu(CEGUI::Window* bar)
{
    // file menu item
    CEGUI::Window* file = mWm->createWindow("WindowsLook/MenuItem");
    file->setText("File");
    bar->addChildWindow(file);

    // file popup
    CEGUI::Window* popup = mWm->createWindow("WindowsLook/PopupMenu");
    file->addChildWindow(popup);

    CEGUI::Window* item;

    //Opens a saved game state
    item = mWm->createWindow("WindowsLook/MenuItem");
    item->setText("New");
    item->subscribeEvent("Clicked", CEGUI::Event::Subscriber(&GUIHUD::fileNew, this));
    popup->addChildWindow(item);

    if (mApp->getIsServer()) {
	    //Opens a saved game state
	    item = mWm->createWindow("WindowsLook/MenuItem");
	    item->setText("Open");
	    item->subscribeEvent("Clicked", CEGUI::Event::Subscriber(&GUIHUD::fileOpen, this));
	    popup->addChildWindow(item);

	    //toggles A* path planning (off by default
	    item = mWm->createWindow("WindowsLook/MenuItem");
	    item->setText("Save");
	    item->subscribeEvent("Clicked", CEGUI::Event::Subscriber(&GUIHUD::fileSave, this));
	    popup->addChildWindow(item);
    }

    // spacer
    item = mWm->createWindow("WindowsLook/MenuItem");
    item->setText("--");
    popup->addChildWindow(item);

    //toggles A* path planning (off by default
    item = mWm->createWindow("WindowsLook/MenuItem");
    item->setText("Toggle path planning");
    item->subscribeEvent("Clicked", CEGUI::Event::Subscriber(&GUIHUD::filePathPlan, this));
    popup->addChildWindow(item);

    if (mApp->getIsServer()) {
        item = mWm->createWindow("WindowsLook/MenuItem");
        item->setText("Send world state");
        item->subscribeEvent("Clicked", CEGUI::Event::Subscriber(&GUIHUD::fileSendState, this));
        popup->addChildWindow(item);
    }

    // spacer
    item = mWm->createWindow("WindowsLook/MenuItem");
    item->setText("--");
    popup->addChildWindow(item);

    // quit item in file menu
    item = mWm->createWindow("WindowsLook/MenuItem");
    item->setText("Quit");
    item->subscribeEvent("Clicked", CEGUI::Event::Subscriber(&GUIHUD::fileQuit, this));
    popup->addChildWindow(item);

    // fullscreen menu item
    CEGUI::Window* demo = mWm->createWindow("WindowsLook/MenuItem");
    demo->setText("View");
    bar->addChildWindow(demo);

    //--
    // view menu popup
    popup = mWm->createWindow("WindowsLook/PopupMenu");
    demo->addChildWindow(popup);

    // fulscreen
    item = mWm->createWindow("WindowsLook/MenuItem");
    item->setText("Fullscreen");
    item->setTooltipText("Hotkey: Space");
    item->subscribeEvent("Clicked", CEGUI::Event::Subscriber(&GUIHUD::viewFullscreen, this));
    popup->addChildWindow(item);

    // spacer
    item = mWm->createWindow("WindowsLook/MenuItem");
    item->setText("--");
    popup->addChildWindow(item);

    // show / hide map
    item = mWm->createWindow("WindowsLook/MenuItem");
    item->setText("Show/hide map");
    item->subscribeEvent("Clicked", CEGUI::Event::Subscriber(&GUIHUD::viewMapDisplay, this));
    popup->addChildWindow(item);

    // show / hide info bar
    item = mWm->createWindow("WindowsLook/MenuItem");
    item->setText("Show/hide info bar");
    item->subscribeEvent("Clicked", CEGUI::Event::Subscriber(&GUIHUD::viewInfoDisplay, this));
    popup->addChildWindow(item);

    //--
    // formations menu item
    CEGUI::Window* formations = mWm->createWindow("WindowsLook/MenuItem");
    formations->setText("Formation");
    bar->addChildWindow(formations);

    // view menu popup
    popup = mWm->createWindow("WindowsLook/PopupMenu");
    formations->addChildWindow(popup);

    //TODO: create menu items from filenames of formation files
    item = mWm->createWindow("WindowsLook/MenuItem");
    item->setText("Circle");
    item->subscribeEvent("Clicked", CEGUI::Event::Subscriber(&GUIHUD::formationClickHandler, this));
    popup->addChildWindow(item);
    item = mWm->createWindow("WindowsLook/MenuItem");
    item->setText("Wedge");
    item->subscribeEvent("Clicked", CEGUI::Event::Subscriber(&GUIHUD::formationClickHandler, this));
    popup->addChildWindow(item);
    item = mWm->createWindow("WindowsLook/MenuItem");
    item->setText("Line");
    item->subscribeEvent("Clicked", CEGUI::Event::Subscriber(&GUIHUD::formationClickHandler, this));
    popup->addChildWindow(item);
    item = mWm->createWindow("WindowsLook/MenuItem");
    item->setText("Column");
    item->subscribeEvent("Clicked", CEGUI::Event::Subscriber(&GUIHUD::formationClickHandler, this));
    popup->addChildWindow(item);

    item = mWm->createWindow("WindowsLook/MenuItem");
    item->setText("--");
    popup->addChildWindow(item);

    item = mWm->createWindow("WindowsLook/MenuItem");
    item->setText("Attack");
    item->subscribeEvent("Clicked", CEGUI::Event::Subscriber(&GUIHUD::formationClickHandler, this));
    popup->addChildWindow(item);
    item = mWm->createWindow("WindowsLook/MenuItem");
    item->setText("Defend");
    item->subscribeEvent("Clicked", CEGUI::Event::Subscriber(&GUIHUD::formationClickHandler, this));
    popup->addChildWindow(item);

}

bool GUIHUD::fileNew(const CEGUI::EventArgs& e) {
	//mApp->Quit();
	mApp->clearScene();
	return true;
}

bool GUIHUD::fileOpen(const CEGUI::EventArgs& e) {
	//TODO CEGUICommonFileDialog
	mApp->loadFile("savegame.xml");
	return true;
}

bool GUIHUD::fileSave(const CEGUI::EventArgs& e) {
	//TODO CEGUICommonFileDialog
	mApp->saveFile("savegame.xml");
	return true;
}

bool GUIHUD::fileQuit(const CEGUI::EventArgs& e) {
	mApp->Quit();
	return true;
}

bool GUIHUD::fileSendState(const CEGUI::EventArgs& e) {
	mApp->sendWorldState(mApp->getControllerId());
	return true;
}

bool GUIHUD::filePathPlan(const CEGUI::EventArgs& e) {
	mApp->togglePathPlanning();
	return true;
}

bool GUIHUD::viewFullscreen(const CEGUI::EventArgs& e) {
	if (mWin->GetFullScreenMode()) {
		//return from full screen mode
		mWin->SetFullScreenMode(false);
	} else {
		//enter full screen mode
		mWin->SetFullScreenMode(true);
	}
	return true;
}

bool GUIHUD::viewMapDisplay(const CEGUI::EventArgs& e) {
	if (map->isVisible()) {
		map->setVisible(false);
		mapWnd->setVisible(false);
	} else {
		map->setVisible(true);
		mapWnd->setVisible(true);
	}
	return true;
}

bool GUIHUD::viewInfoDisplay(const CEGUI::EventArgs& e) {
	if (toolWnd->isVisible()) {
		toolWnd->setVisible(false);
		toolWndBg->setVisible(false);
		st->setVisible(false);
	} else {
		toolWnd->setVisible(true);
		toolWndBg->setVisible(true);
		st->setVisible(true);
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////
void GUIHUD::TickHUD()
{
	//
}

void GUIHUD::setHitPos(osg::Vec3 p) {
	std::ostringstream oss;
	oss << p;
	st->setText(oss.str());
}

//////////////////////////////////////////////////////////////////////////
/*void GUIHUD::UpdateStaticText(CEGUI::StaticText *textControl, char *newText,
                                  float red, float blue, float green, float x, float y)
{
   if (textControl != NULL)
   {
      // text and color
      if (newText != NULL && textControl->getText() != std::string(newText))
      {
         textControl->setText(newText);
         if (red >= 0.00 && blue >= 0.0 && green >= 0.0)
            textControl->setTextColours(CEGUI::colour(red, blue, green));
      }
      // position
      if (x > 0.0 && y > 0.0)
      {
         CEGUI::Point position = textControl->getPosition();
         CEGUI::Point newPos(x, y);
         if (position != newPos)
            textControl->setPosition(newPos);
      }
   }
}*/

//////////////////////////////////////////////////////////////////////////
/*CEGUI::StaticText * GUIHUD::CreateText(const std::string &name, CEGUI::StaticImage *parent, const std::string &text,
                                 float x, float y, float width, float height)
{
   CEGUI::WindowManager *wm = CEGUI::WindowManager::getSingletonPtr();

   // create base window and set our default attribs
   CEGUI::StaticText* result = static_cast<CEGUI::StaticText*>(wm->createWindow("WindowsLook/StaticText", name));
   parent->addChildWindow(result);
   result->setMetricsMode(CEGUI::Absolute);
   result->setText(text);
   result->setPosition(CEGUI::Point(x, y));
   result->setSize(CEGUI::Size(width, height));
   result->setFrameEnabled(false);
   result->setBackgroundEnabled(false);
   result->setHorizontalAlignment(CEGUI::HA_LEFT);
   result->setVerticalAlignment(CEGUI::VA_TOP);
   result->show();

   return result;
}
*/

MarkerType GUIHUD::CreateMarker(std::string image_filename, double size)
{
	CEGUI::Imageset* imageset;

	if ( !ISm->isImagesetPresent(image_filename) )
	{
		imageset = ISm->createImagesetFromImageFile(image_filename, image_filename);
		imageset->setAutoScalingEnabled (false);
	}

	// get the set if it was not loaded now
	imageset = ISm->getImageset(image_filename);

	float marker_width = imageset->getImageWidth("full_image");
	float marker_height = imageset->getImageHeight("full_image");

	MarkerType marker;

	std::ostringstream ss;
	ss << "marker" << m_Markers.size();

	marker = mWm->createWindow("WindowsLook/StaticImage", ss.str());
	m_Markers.push_back(marker);
	marker->setRiseOnClickEnabled(false);

	mMainWindow->addChildWindow(marker);

	// measure position from the lower right corner
	marker->setHorizontalAlignment(CEGUI::HA_RIGHT);
	marker->setVerticalAlignment(CEGUI::VA_BOTTOM);

	// set size according to the passed parameter
	marker->setSize(CEGUI::UVector2(CEGUI::UDim(size,marker_width), CEGUI::UDim(size,marker_height)));

	// disable frame and standard background
	marker->setProperty("FrameEnabled", "false");
	marker->setProperty("BackgroundEnabled", "false");

	// set the background image
	ss.str(std::string());
	ss.clear();

	ss << "set:" << image_filename << " image:full_image";

	marker->setProperty("Image", ss.str());
	marker->setAlpha(1.0f);

	return marker;
}

void GUIHUD::SetPosOfMarker(MarkerType marker, double xpos, double ypos)
{
	// first find the marker
	std::vector<MarkerType>::iterator it;
	for (it = m_Markers.begin(); it != m_Markers.end(); it++)
	{
		if ( (*it) == marker)
		{
			CEGUI::UDim width =(*it)->getWidth();
			CEGUI::UDim height =(*it)->getHeight();
			double tr_xpos = FRAME_OF_MMAP + (1-xpos)*SIZE_OF_MMAP - width.d_offset/2;
			double tr_ypos = FRAME_OF_MMAP + ypos*SIZE_OF_MMAP - height.d_offset/2;
			(*it)->setPosition( CEGUI::UVector2(CEGUI::UDim(0,-tr_xpos), CEGUI::UDim(0,-tr_ypos)) );
			return;
		}
	}
	LOG_WARNING("Marker not found.");
}

void GUIHUD::RemoveMarker(MarkerType marker)
{
	// first find the marker
	std::vector<MarkerType>::iterator it;
	for (it = m_Markers.begin(); it != m_Markers.end(); it++)
	{
		if ( (*it) == marker)
		{
			mWm->destroyWindow(*it);
			m_Markers.erase(it);
			return;
		}
	}
	LOG_WARNING("Marker not found.");
}

void GUIHUD::createUnitInfoDisplay(Unit* unit)
{
	std::ostringstream oss;
	oss << "unitInfoBg" << m_NumUnitDisplays;
	CEGUI::Window* toolWndBg = mWm->createWindow("WindowsLook/Static", oss.str());
	mMainWindow->addChildWindow(toolWndBg);
	toolWndBg->setPosition(CEGUI::UVector2(CEGUI::UDim(0,10), CEGUI::UDim(0,((m_NumUnitDisplays+1)*mFont->getLineSpacing(1.2f))+(10*(m_NumUnitDisplays+1)))));
	toolWndBg->setSize(CEGUI::UVector2(CEGUI::UDim(0,150), CEGUI::UDim(0,mFont->getLineSpacing(1.2f))));
	toolWndBg->setProperty("FrameEnabled", "false");
	toolWndBg->setAlpha(0.5f);
	toolWndBg->setRiseOnClickEnabled(false);

	oss.str("");
	oss << "unitNameText" << m_NumUnitDisplays;
	CEGUI::Window* st = mWm->createWindow("WindowsLook/Button", oss.str());
	mMainWindow->addChildWindow(st);
	st->setPosition(CEGUI::UVector2(CEGUI::UDim(0,12), CEGUI::UDim(0,((m_NumUnitDisplays+1)*mFont->getLineSpacing(1.2f))+(10*(m_NumUnitDisplays+1)))));
	st->setSize(CEGUI::UVector2(CEGUI::UDim(0,75), CEGUI::UDim(0,mFont->getLineSpacing(1.0f))));
	st->setText(unit->GetName());

	if (unit->getSide() == RED)
		st->setProperty("NormalTextColour", "FFFF1010");
	else if (unit->getSide() == BLUE)
		st->setProperty("NormalTextColour", "FF1010FF");

	// disable frame and background on static control
	//st->setProperty("FrameEnabled", "false");
	//st->setProperty("BackgroundEnabled", "false");
	st->subscribeEvent("Clicked", CEGUI::Event::Subscriber(&GUIHUD::unitLabelClick, this));

	oss.str("");
	oss << "unitHealthText" << m_NumUnitDisplays;
	CEGUI::Window* ht = mWm->createWindow("WindowsLook/ProgressBar", oss.str());
	mMainWindow->addChildWindow(ht);
	ht->setPosition(CEGUI::UVector2(CEGUI::UDim(0,87), CEGUI::UDim(0,((m_NumUnitDisplays+1)*mFont->getLineSpacing(1.2f))+(10*(m_NumUnitDisplays+1)))));
	ht->setSize(CEGUI::UVector2(CEGUI::UDim(0,40), CEGUI::UDim(0,mFont->getLineSpacing(1.0f))));
	//ht->setText("100");
	// disable frame and background on static control
	//ht->setProperty("FrameEnabled", "false");
	//ht->setProperty("BackgroundEnabled", "false");
	((CEGUI::ProgressBar*)ht)->setProgress(1);
	unit->setHealthWindow(ht);

	CEGUI::Window* rt = 0;
	UnitFiring* my_unitfiring = dynamic_cast<UnitFiring*>(unit);
	if (my_unitfiring != 0)
	{
		oss.str("");
		oss << "unitReloadText" << m_NumUnitDisplays;
		rt = mWm->createWindow("WindowsLook/StaticText", oss.str());
		mMainWindow->addChildWindow(rt);
		rt->setPosition(CEGUI::UVector2(CEGUI::UDim(0,130), CEGUI::UDim(0,((m_NumUnitDisplays+1)*mFont->getLineSpacing(1.2f))+(10*(m_NumUnitDisplays+1)))));
		rt->setSize(CEGUI::UVector2(CEGUI::UDim(0,30), CEGUI::UDim(0,mFont->getLineSpacing(1.0f))));
		rt->setText("0");
		// disable frame and background on static control
		rt->setProperty("FrameEnabled", "false");
		rt->setProperty("BackgroundEnabled", "false");
		rt->setRiseOnClickEnabled(false);

		my_unitfiring->setReloadWindow(rt);
	}
	m_NumUnitDisplays++;
}

void GUIHUD::removeUnitInfoDisplays() {
	for (unsigned int i=0; i < m_NumUnitDisplays; i++) {
		std::ostringstream oss;
		oss << i;
		try {
			if (mWm->isWindowPresent("unitInfoBg"+oss.str())) {
				mWm->destroyWindow(mWm->getWindow("unitInfoBg"+oss.str()));
			}
			if (mWm->isWindowPresent("unitNameText"+oss.str())) {
				mWm->destroyWindow(mWm->getWindow("unitNameText"+oss.str()));
			}
			if (mWm->isWindowPresent("unitHealthText"+oss.str())) {
				mWm->destroyWindow(mWm->getWindow("unitHealthText"+oss.str()));
			}
			if (mWm->isWindowPresent("unitReloadText"+oss.str())) {
				mWm->destroyWindow(mWm->getWindow("unitReloadText"+oss.str()));
			}
		} catch (...) {
			std::cout << "Can't find some window " << i << std::endl;
		}
	}
	m_NumUnitDisplays = 0;
}

void GUIHUD::setHealth(CEGUI::Window *w, float health) {
	std::ostringstream oss;

	if (health == INFINITY)
	{
		mWm->destroyWindow(w);
		return;
	}

	if (health > 0) {
		oss << std::fixed << std::setprecision(0) << health;
		((CEGUI::ProgressBar*)w)->setProgress(health/100);
	} else {
		oss << "xx";
		((CEGUI::ProgressBar*)w)->setProgress(0);
	}
	//w->setText(oss.str());

}

void GUIHUD::setReload(CEGUI::Window *w, float reload) {
	std::ostringstream oss;

	if (reload == INFINITY)
	{
		w->setText("DTD");
		return;
	}

	if (reload < 0) reload = 0;
	oss << std::fixed << std::setprecision(2) << reload;
	w->setText(oss.str());
}

bool GUIHUD::unitLabelClick(const CEGUI::EventArgs& e) {
	CEGUI::Window *w = ((const CEGUI::WindowEventArgs&)e).window;
	//lookup unit from name
	theApp->FocusOnUnit(w->getText().c_str());
	return true;
}

void GUIHUD::ShowPausedRed(bool val)
{
	m_PausedRed->setVisible(val);
}

void GUIHUD::ShowPausedBlue(bool val)
{
	m_PausedBlue->setVisible(val);
}

bool GUIHUD::formationClickHandler(const CEGUI::EventArgs& e) {
	CEGUI::Window *menuItem = (dynamic_cast<const CEGUI::WindowEventArgs*>(&e))->window;
	CEGUI::String item = menuItem->getText();
	//std::cout << item << std::endl;

	theApp->setSelectedFormation(std::string(item.c_str()));

	return true;
}

void GUIHUD::SetTime( signed int total_sec, signed int goal_sec )
  {
        std::ostringstream message;

        signed int total_min = floor(total_sec/60);
        signed int goal_min = floor(goal_sec/60);

        if (goal_sec != 0)
          {
            message << "Time on goal: " << goal_min << "m " << goal_sec-60*goal_min << "s" << std::endl;
          }
        if (total_sec != 0)
          {
            message << "Remaining: " << total_min << "m " << total_sec-60*total_min << "s" << std::endl;
          }

        m_Time->setText(message.str());
  }

void GUIHUD::ShowGameOverMessage(std::string message)
{
    CEGUI::Window* game_over_text = mWm->createWindow("WindowsLook/StaticText", "gameOverText");
    mMainWindow->addChildWindow(game_over_text);
    game_over_text->setText(message);
    game_over_text->setFont(m_HugeFont);
    game_over_text->setSize(CEGUI::UVector2(CEGUI::UDim(0, m_HugeFont->getTextExtent(message) ), CEGUI::UDim(0,m_HugeFont->getLineSpacing(1.0f))));
    game_over_text->setPosition( CEGUI::UVector2(CEGUI::UDim(0.5f,-m_HugeFont->getTextExtent(message)/2), cegui_reldim(0.4f) ) );
    // disable frame and background on static control
    game_over_text->setProperty("FrameEnabled", "false");
    game_over_text->setProperty("BackgroundEnabled", "false");
    game_over_text->setProperty("TextColours", "tl:FFFFFFFF tr:FFFFFFFF bl:FF000000 br:FF000000");

    game_over_text->setVisible(true);
}

void GUIHUD::SetTimeScale(double timeScale, double desiredTimeScale, double requestedTimeScale) {
	std::ostringstream oss;

    oss << std::fixed << std::setprecision(2) << timeScale << "x" << std::endl;

    if (desiredTimeScale > 0 && desiredTimeScale != timeScale)
    {
    	oss << "Desired: " << std::fixed << std::setprecision(2) << desiredTimeScale << "x" << std::endl;
    }
    if (requestedTimeScale > 0 && requestedTimeScale != timeScale)
    {
    	oss << "Requested: " << std::fixed << std::setprecision(2) << requestedTimeScale << "x" << std::endl;
    }

    m_TimeScale->setText(oss.str());
}
