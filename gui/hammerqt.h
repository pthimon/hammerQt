#ifndef HAMMERQT_H
#define HAMMERQT_H

#include <osg/Vec3>
#include <QString>
#include <QtGui/QMainWindow>
#include <QGraphicsItemGroup>
#include <qwt5/qwt_plot.h>
#include <qwt5/qwt_plot_curve.h>
#include <dtCore/refptr.h>

#include "../common.h"
#include "OSGAdapterWidget.h"
#include "ui_hammerqt.h"
#include "guihud.h"

class Unit;
class UnitGroup;
class UnitMarker;
class GroupMarker;
class HypothesisResult;
class Hypothesis;

class ResultsPlot {
public:
	ResultsPlot(QwtPlot* plot, int num) {
		//create curves
		for (int i=0; i < num; i++) {
			QwtPlotCurve *curve = new QwtPlotCurve("Curve");
			int index = mCurves.size();
			mCurves.push_back(curve);
			std::vector<double> x;
			x.push_back(0);
			std::vector<double> y;
			y.push_back(0);
			mXAxes.push_back(x);
			mYAxes.push_back(y);
			curve->attach(plot);
			QColor c;
			c.setHsvF(((float)i)/num, 1-((float)(index%5))/5, 1);
			curve->setPen(QPen(c));
		}
	}
	void addResult(unsigned int index, double time, double conf) {
		if (index < mXAxes.size() && index < mYAxes.size()) {
			mXAxes[index].push_back(time);
			mYAxes[index].push_back(conf);
			mCurves[index]->setRawData(&mXAxes[index][0], &mYAxes[index][0], mXAxes[index].size());
		} else {
			LOG_ERROR("Not enough curves have been created!")
		}
	}
	QwtPlotCurve* getCurve(int index) {
		return mCurves[index];
	}
private:
	std::vector<QwtPlotCurve*> mCurves;
	std::vector<std::vector<double> > mXAxes;
	std::vector<std::vector<double> > mYAxes;
};

class hammerQt : public QMainWindow, public GUIHUD
{
    Q_OBJECT

public:
    hammerQt(QWidget *parent = 0);
    ~hammerQt();

	//unneeded methods with qt gui
	MarkerType CreateMarker(std::string, double) { return NULL; }
	void SetPosOfMarker(MarkerType, double, double) {}
	void RemoveMarker(MarkerType) {}
    void createUnitInfoDisplay(Unit*) {}
    void removeUnitInfoDisplays() { clearUnits(); }
    void setHealth(CEGUI::Window*, float) {}
    void setReload(CEGUI::Window*, float) {}

    //overloaded methods
    void setHitPos(osg::Vec3 p);

    void ShowPausedRed(bool val);
    void ShowPausedBlue(bool val);

    void ShowGameOverMessage(std::string message);

    void SetTime( signed int total_sec, signed int goal_sec );
    void SetTimeScale(double timeScale, double desiredTimeScale, double requestedTimeScale);

    //Qt specific

    dtQt::OSGAdapterWidget* GetGLWidget() { return mGLWidget; }

	void initialiseMap(std::string mapName);
	void updateMap(std::vector<Unit*>& units);

    void initialiseUnits(std::vector<Unit* > units);
    void removeUnit(Unit *unit);
    void clearUnits();

    void updateUnits(std::vector<Unit*> units);
	void updateUnitsTable(std::vector<Unit*>& units);
	void updateUnitGroups(std::vector<UnitGroup*> groups);
	void updateObservedUnitGroups(std::vector<UnitGroup*> groups);

	void addHypothesis(Hypothesis* h);
	void showHypothesisResults(int id, std::vector<HypothesisResult*> r);
	void updateResults(std::vector<dtCore::RefPtr<Hypothesis> > hypotheses);

    Ui::hammerQtClass ui;
    QActionGroup *formationGroup;
    QActionGroup *manoeuvreGroup;

signals:
	void unitSelected(QString unitId);

protected:
	QPointF coordToPoint(float x, float y);
	osg::Vec2 coordToPoint(osg::Vec2 c);
	float distToPixels(float dist);
	QGraphicsPathItem *createPath(std::vector<osg::Vec2>& path);

private slots:
	void on_zoomSlider_valueChanged(int value);
	void on_action_Replay_triggered();
	void on_actionExit_triggered();
	void on_actionDemonstration_triggered();
	void on_unitsTable_itemClicked(QTableWidgetItem * item );
	void on_action_Fullscreen_triggered(bool checked);
	void on_plotCheckBox_stateChanged(int state);
	void updateMapParams(std::vector<osg::Vec3> params);
	void updateClusters(std::vector<std::vector<int> > clusters);
	void clearPaths();
	void addPath(std::vector<osg::Vec2> paths);

private:
    dtQt::OSGAdapterWidget *mGLWidget;
    QGraphicsScene* mMap;
    std::vector< Unit* > mUnits;
    std::map<int,UnitMarker*> mUnitMarkers;
    QGraphicsItemGroup* mPathLayer;
    std::vector<QGraphicsItem*> mPaths;
    QGraphicsItemGroup* mGroupLayer;
    std::vector<GroupMarker*> mUnitGroupMarkers;
    QGraphicsItemGroup* mObservedGroupLayer;
    std::vector<GroupMarker*> mObservedUnitGroupMarkers;
    QGraphicsItemGroup* mHypothesisLayer;
    std::map<int, QGraphicsItemGroup*> mHypothesisLayerItems;

    QwtPlot *mResultsPlot;
    std::map<int,ResultsPlot* > mResultsPlotCurves;
    bool mPlotResults;
};

#endif // HAMMERQT_H
