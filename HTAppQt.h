#ifndef HTAPPQT_H_
#define HTAPPQT_H_

#include <QObject>
#include <QAction>

#include "HTapp.h"
#include "gui/hammerqt.h"
#include "units/UnitObserver.h"

class HypothesisManager;
class Observations;

class HTAppQt : public QObject, public HTApplication
{
	Q_OBJECT

public:
	HTAppQt(const std::string& configFilename, const std::string& hostName, int port, bool wait_for_client, const std::string& scenarioFilename, int id, std::string& replayFile, std::string& configFile);
	virtual ~HTAppQt();

	virtual void initGUI();
	virtual void Init(hammerQt* gui);
	virtual void CreateActors();
	virtual void InitHypothesisManager();
	virtual void PreFrameGUI(const double);
	virtual void PostFrame(const double);
	void setGUI(hammerQt* gui) { hudGUI = mQtGUI = gui; }

protected slots:
	void unitSelected(QString unitId);
	void onNew();
	void onOpen();
	void onSave();
	void onPathPlanning(bool);
	void onFormation(QAction *);
	void onManoeuvre(QAction *);
	void onSaveAll();
public slots:
	void onLaunchHypotheses();


protected:
	void makeConnections();
	virtual void RemoveUnit(Unit* unit, bool clear = false);

	//Overrides the default drag rect that uses the X api.
	virtual void drawDragRect(osg::Vec2 pressPoint, osg::Vec2 dragPoint);

	hammerQt* mQtGUI;
};

#endif /*HTAPPQT_H_*/
