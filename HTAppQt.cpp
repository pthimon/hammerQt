#include "HTAppQt.h"

#include <QtOpenGL/QGLContext>
#include <QtGui/QDockWidget>
#include <QPainter>

#include <dtCore/system.h>
#include <dtCore/deltawin.h>

#include <osg/GraphicsContext>
#include <osgViewer/Viewer>

#include "gui/OSGAdapterWidget.h"
#include "gui/hammerqt.h"
#include "eventlog.h"
#include "hypotheses/HypothesisManager.h"
#include "hypotheses/HypothesisClient.h"
#include "hypotheses/DirectionHypothesis.h"
#include "hypotheses/TargetHypothesis.h"
#include "models/inc/Targets.h"
#include "units/UnitObserver.h"
#include "models/inc/Observations.h"

HTAppQt::HTAppQt(const std::string& configFilename, const std::string& hostName, int port, bool wait_for_client, const std::string& scenarioFilename, int id, std::string& replayFile, std::string& configFile):
	HTApplication( configFilename, hostName, port, wait_for_client, scenarioFilename, id, replayFile, configFile)
{
	qRegisterMetaType<std::vector<osg::Vec3> >("std::vector<osg::Vec3>");
	qRegisterMetaType<std::vector<std::vector<int> > >("std::vector<std::vector<int> >");
	qRegisterMetaType<std::vector<osg::Vec2> >("std::vector<osg::Vec2>");
	qRegisterMetaType<Hypothesis*>("Hypothesis*");
}

HTAppQt::~HTAppQt()
{
   dtCore::System::GetInstance().Stop();
}

/**
 * Initialises the delta3d app and also calls initGUI to setup the Qt-based GUI
 */
void HTAppQt::Init(hammerQt* gui) {
	setGUI(gui);

	//call base class to initialise delta3d
	HTApplication::Init();

	mQtGUI->initialiseMap(m_MapName);

	dtCore::System::GetInstance().Start();
}

void HTAppQt::CreateActors() {
	HTApplication::CreateActors();

	std::vector<Unit*> units = mTargets->getUnits();
	mQtGUI->initialiseUnits(units);
}

void HTAppQt::InitHypothesisManager() {
	//create hypothesis manager (creates server on this host on another port)
	HypothesisManager::create(getHostname(), 4449, mTargets, mObservations, mQtGUI);
	if (!mLaunchClients) {
		for (unsigned int i=0; i < mNumClients; i++) {
			new HypothesisClient(i+100);
		}
	}
}

void HTAppQt::initGUI() {
	//need to set the current context so that all the open gl stuff in osg can initialize.
	mQtGUI->GetGLWidget()->SetGraphicsWindow(*GetWindow()->GetOsgViewerGraphicsWindow());

	makeConnections();
}

void HTAppQt::makeConnections() {
	//setup qt signals/slots here to communicate to/from main GUI window
	connect(mQtGUI->ui.actionNew, SIGNAL(triggered()), this, SLOT(onNew()));
	connect(mQtGUI->ui.action_Open, SIGNAL(triggered()), this, SLOT(onOpen()));
	connect(mQtGUI->ui.action_Save, SIGNAL(triggered()), this, SLOT(onSave()));
	connect(mQtGUI->ui.action_Path_planning, SIGNAL(triggered(bool)), this, SLOT(onPathPlanning(bool)));
	connect(mQtGUI->formationGroup, SIGNAL(triggered(QAction*)), this, SLOT(onFormation(QAction*)));
	connect(mQtGUI->manoeuvreGroup, SIGNAL(triggered(QAction*)), this, SLOT(onManoeuvre(QAction*)));
	connect(mQtGUI, SIGNAL(unitSelected(QString)), this, SLOT(unitSelected(QString)));
	connect(mQtGUI->ui.action_Launch, SIGNAL(triggered()), this, SLOT(onLaunchHypotheses()));
	connect(mQtGUI->ui.action_Save_2, SIGNAL(triggered()), this, SLOT(onSaveAll()));
}

void HTAppQt::PreFrameGUI(const double deltaFrameTime) {
	HTApplication::PreFrameGUI(deltaFrameTime);

	//update the unit markers on the map
	std::vector<Unit*> units = mTargets->getUnits();
	mQtGUI->updateUnits(units);
	//update the group boundaries
	std::vector<UnitGroup*> groups = mTargets->getUnitGroups();
	mQtGUI->updateUnitGroups(groups);
	//update the observed group gradients
	std::vector<UnitGroup*> observedgroups = mTargets->getObservedUnitGroups(ACTIVE);
	mQtGUI->updateObservedUnitGroups(observedgroups);
}

void HTAppQt::PostFrame(const double deltaFrameTime) {
	HTApplication::PostFrame(deltaFrameTime);
	//perform any 2D Qt-based drawing here with a QPainter
}

void HTAppQt::RemoveUnit(Unit *unit, bool /*clear*/) {
	mQtGUI->removeUnit(unit);

	//call base class
	HTApplication::RemoveUnit(unit);
}

void HTAppQt::drawDragRect(osg::Vec2 pressPoint, osg::Vec2 dragPoint) {
	QPainter painter(mQtGUI->GetGLWidget());
	painter.setBrush(QBrush(QColor(150,150,150,100)));
	painter.drawRect(pressPoint[0],pressPoint[1],dragPoint[0]-pressPoint[0],dragPoint[1]-pressPoint[1]);
	painter.end();
}

void HTAppQt::unitSelected(QString unitName) {
	FocusOnUnit(unitName.toStdString());
}

void HTAppQt::onNew() {
	restartScenario();
}

void HTAppQt::onOpen() {
	loadFile("savegame.xml");
}

void HTAppQt::onSave() {
	saveFile("savegame.xml");
}

void HTAppQt::onPathPlanning(bool checked) {
	mPathPlanning = checked;
}

void HTAppQt::onFormation(QAction *action) {
	setSelectedFormation(action->iconText().toStdString());
}

void HTAppQt::onManoeuvre(QAction *action) {
	setSelectedFormation(action->iconText().toStdString());
}

void HTAppQt::onLaunchHypotheses() {
	//start the hypotheses
	launchHypotheses();
	//change the gui if the launch worked
	HypothesisManager* manager = HypothesisManager::get();
	if (manager) {
		//show parts of the gui relevant to hypotheses
		mQtGUI->ui.hypothesesDockWidget->show();
		mQtGUI->ui.tabWidget->setTabEnabled(2, true);
		mQtGUI->ui.action_Save_2->setEnabled(true);
		mQtGUI->ui.action_Launch->setEnabled(false);
	}
}

void HTAppQt::onSaveAll() {
	saveAll();
}
