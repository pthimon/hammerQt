#ifndef HTAPP_H_
#define HTAPP_H_

#include "common.h"
#pragma GCC diagnostic ignored "-Wextra"

#include <dtCore/flymotionmodel.h>
#include <dtCore/mouse.h>
#include <dtCore/genericmouselistener.h>
#include <dtCore/clouddome.h>
#include <dtCore/cloudplane.h>

#include <dtCore/camera.h>
#include <dtCore/keyboard.h>
#include <dtCore/generickeyboardlistener.h>

#pragma GCC diagnostic warning "-Wextra"

#include "application_HT.h"
#include "models/InverseModelFormation.h"

class HTApplication : public Application_HT
{
   public:

    HTApplication( const std::string& configFilename,
		const std::string& hostName, int port, bool wait_for_client, const std::string& scenarioFilename, int id, std::string& replayFile, std::string& configFile);

	virtual void CreateEnvironment();
	virtual void Config();
	virtual void initGUI();

    virtual bool KeyPressed(const dtCore::Keyboard *keyBoard, int kc);
	virtual bool KeyReleased(const dtCore::Keyboard *keyBoard, int kc);

    void ShowStatistics();
    virtual void Quit();

    /**
     * Override of dtABC::PreFrame
     * Called automatically before each frame
     */
    virtual void PreFrameGUI( const double deltaFrameTime );
    virtual void PreFrame( const double deltaFrameTime );
    virtual void PostFrame( const double deltaFrameTime );

    //Mouse events
    bool MouseMoved(const dtCore::Mouse* mouse, float x, float y);
    bool MouseButtonPressed(const dtCore::Mouse* mouse, dtCore::Mouse::MouseButton button);
    bool MouseButtonReleased(const dtCore::Mouse* mouse, dtCore::Mouse::MouseButton button);
    bool MouseButtonClicked(const dtCore::Mouse* mouse, dtCore::Mouse::MouseButton button, int click_count);
    bool MouseDragged(const dtCore::Mouse* mouse, float x, float y);

	//Mouse hit tests
	Unit *unitHitTest(float x, float y);
    osg::Vec3 terrainHitTest(float x, float y);

	// focuses on the unit with the given name
	void FocusOnUnit(const std::string & name);

	void moveCameraToLookAt(osg::Vec3 pos);
	void setSelectedUnit(Unit *unit);

	virtual void BroadcastEvent(Event::EventType e, Unit *unit1 = NULL, Unit *unit2 = NULL, osg::Vec3 pos = osg::Vec3(NAN,NAN,NAN));
	virtual void ReceiveEvent(Event::EventType event, std::vector<std::string>& participants, std::vector<double>& data);

	virtual void displayUnitHealth(Unit *unit, float health);
	virtual void displayUnitReload(UnitFiring *unit, double reload);

	virtual void zoomUpdate(float dist);
	void setSelectedFormation(std::string formation);

	virtual void clearScene();

	virtual bool isHeadless();
	void requestMultiplyTimeScale(double scale);

	virtual void EndGame( UnitSide winner );
	virtual void EndGameGUI();

	virtual GUIHUD* getGUI() { return hudGUI.get(); }

	void setLookAtUnit(Unit* unit);

protected:

    ~HTApplication();
    virtual void AddUnit(Unit* unit);
    virtual void RemoveUnit(Unit* unit, bool clear = false);

    //draws the drag rect using the X api. Override for other drawing methods.
    virtual void drawDragRect(osg::Vec2 pressPoint, osg::Vec2 dragPoint);

	// Motion Model
	//dtCore::RefPtr<dtCore::OrbitMotionModel> m_MotionModel;
	dtCore::RefPtr<MapMotionModel> m_MotionModel;

	dtCore::RefPtr<dtCore::GenericMouseListener> m_MouseListener;

	osg::Vec3 m_MousePosition;
	Unit *m_MouseOverUnit;
	std::vector< dtCore::RefPtr<Unit> > m_SelectedUnits;
	float m_MouseX, m_MouseY;

	bool mPressed;
	bool mDragging;
	osg::Vec2 mPressPoint;
	osg::Vec2 mDragPoint;

	int m_KeyModifier;

	bool m_RightDeselect;

	double mRequestedTimeScale;
	double mDesiredTimeScale;

	// GUI
	dtCore::RefPtr<GUIHUD> hudGUI;

	Unit* mCameraLookAtUnit;

};

#endif /*HTAPP_H_*/
