#ifndef DELTA_GUIHUD
#define DELTA_GUIHUD

#include <dtABC/application.h>
#include <osg/Referenced>
#include <osg/Vec3>

#include <vector>

#include <dtGUI/dtgui.h>

#include "../HTAppBase.h"
#include "../common.h"
#include "camtriangle.h"
#include "compass2.h"

class HTApplication;
class Unit;

#define HUDCONTROLMAXTEXTSIZE 100

namespace dtActors
{
   class TaskActorProxy;
}

/**
 * Exception that may be thrown by the HUD.
 */
class GUIHUDException : public dtUtil::Enumeration
{
   DECLARE_ENUM(GUIHUDException);
   public:
      static GUIHUDException INIT_ERROR;
      static GUIHUDException RUNTIME_ERROR;
   private:
      GUIHUDException(const std::string &name) : dtUtil::Enumeration(name)
      {
         AddInstance(this);
      }
};

/**
 * HUD State enumeration - what info is the HUD showing.
 */
class HUDState : public dtUtil::Enumeration
{
   DECLARE_ENUM(HUDState);
   public:
      static HUDState MINIMAL;
      static HUDState MEDIUM;
      static HUDState MAXIMUM;
      static HUDState NONE;
      static HUDState HELP;
   private:
      HUDState(const std::string &name) : dtUtil::Enumeration(name)
      {
         AddInstance(this);
      }
};

/**
 * This class draws the HUD for the testAAR with using CEGUI.  It draws
 * status information like AAR state (record, playback, idle), sim time,
 * speed factor, num messages, and other help info etc...
 */
class GUIHUD : public osg::Referenced
{
   public:

	  /**
	   * Default constructor
	   */
	  GUIHUD() {}

      /**
       * Constructs the test application.
       */
      GUIHUD(HTApplication *app, dtCore::DeltaWin *win, std::string map_name);

      /**
       * Sets up the basic GUI.
       */
      void SetupGUI(dtCore::DeltaWin *win, std::string map_name);

      dtCore::RefPtr<dtGUI::CEUIDrawable> GetGUIDrawable() { return mGUI; }

      void createMenu(CEGUI::Window* bar);

      void TickHUD();

	  virtual void setHitPos(osg::Vec3 p);

	  /** Create a new marker and return a unique identifier to it for later
	   * referencing.
	   * @params image_filename A filename holding the image for the marker.
	   * @params size A relative scale for the marker size.
	   */
	  virtual MarkerType CreateMarker(std::string image_filename, double size);
	  /** Set the position of the given marker on the minimap.
	   * Coords are given in relative coordinates to map size. (0,0) is upper left and
	   * (1,1) is lower right.
	   */
	  virtual void SetPosOfMarker(MarkerType marker, double xpos, double ypos);
	  /** Remove the marker.
	   * @params marker the identifier of the marker
	   */
	  virtual void RemoveMarker(MarkerType marker);

	//text
      CEGUI::Window* st;
      CEGUI::Window* mDebugText;
      CEGUI::Window* toolWnd;
      CEGUI::Window* toolWndBg;
      CEGUI::Window* mapWnd;
      CEGUI::Window* map;

      std::vector<MarkerType> m_Markers;

      //menu events
      bool fileNew(const CEGUI::EventArgs& e);
      bool fileOpen(const CEGUI::EventArgs& e);
      bool fileSave(const CEGUI::EventArgs& e);
      bool fileQuit(const CEGUI::EventArgs& e);
      bool fileSendState(const CEGUI::EventArgs& e);
      bool filePathPlan(const CEGUI::EventArgs& e);
      bool viewFullscreen(const CEGUI::EventArgs& e);
      bool viewMapDisplay(const CEGUI::EventArgs& e);
      bool viewInfoDisplay(const CEGUI::EventArgs& e);
      bool formationClickHandler(const CEGUI::EventArgs& e);
      //button events
      bool unitLabelClick(const CEGUI::EventArgs& e);

      virtual void createUnitInfoDisplay(Unit* unit);
      virtual void removeUnitInfoDisplays();
      virtual void setHealth(CEGUI::Window *w, float health);
      virtual void setReload(CEGUI::Window *w, float reload);

      virtual void ShowPausedRed(bool val);
      virtual void ShowPausedBlue(bool val);

      virtual void ShowGameOverMessage(std::string message);

      /** This function sets the time counter shown in the upper right corner.
      * If timer is 0, it is not shown.
      */
      virtual void SetTime( signed int total_sec, signed int goal_sec );

      //shows the current, players desired scale and other players requested scale in upper right
      virtual void SetTimeScale(double timeScale, double desiredTimeScale, double requestedTimeScale);

   protected:

      /**
       * Destroys the test application.
       */
      virtual ~GUIHUD();

   private:

      dtCore::RefPtr<dtCore::Compass> m_Compass;
      dtCore::RefPtr<CamTriangle> m_CamTriangle;

      dtCore::DeltaWin *mWin;
      CEGUI::Window *mMainWindow;
      dtCore::RefPtr<dtGUI::CEUIDrawable> mGUI;
	  HTApplication *mApp;

      CEGUI::Font* mFont;
      CEGUI::Font* m_HugeFont;
      CEGUI::WindowManager *mWm;
      CEGUI::ImagesetManager *ISm;

      CEGUI::Window* m_PausedBlue;
      CEGUI::Window* m_PausedRed;

      CEGUI::Window* m_Time;
      CEGUI::Window* m_TimeScale;

      unsigned int m_NumUnitDisplays;
};

#endif
