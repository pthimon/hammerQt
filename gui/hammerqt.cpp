/****************************************************************************
**
** Trolltech hereby grants a license to use the Qt/Eclipse Integration
** plug-in (the software contained herein), in binary form, solely for the
** purpose of creating code to be used with Trolltech's Qt software.
**
** Qt Designer is licensed under the terms of the GNU General Public
** License versions 2.0 and 3.0 ("GPL License"). Trolltech offers users the
** right to use certain no GPL licensed software under the terms of its GPL
** Exception version 1.2 (http://trolltech.com/products/qt/gplexception).
**
** THIS SOFTWARE IS PROVIDED BY TROLLTECH AND ITS CONTRIBUTORS (IF ANY) "AS
** IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
** TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
** PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
** OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
** EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
** PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
** LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
** NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
** SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** Since we now have the GPL exception I think that the "special exception
** is no longer needed. The license text proposed above (other than the
** special exception portion of it) is the BSD license and we have added
** the BSD license as a permissible license under the exception.
**
****************************************************************************/

#include "hammerqt.h"
//#include "../units/UnitFiring.h"

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QProgressBar>
#include <QStringList>
#include <QHeaderView>
#include <QGLWidget>
#include <QMessageBox>
#include <QColorGroup>
#include <QColor>
#include <QFileDialog>
#include <qwt5/qwt_plot_canvas.h>

#include "../units/Unit.h"
#include "../units/UnitGroup.h"
#include "UnitMarker.h"
#include "GroupMarker.h"
#include "../hypotheses/HypothesisResults.h"
#include "../hypotheses/Hypothesis.h"
#include "../hypotheses/TargetHypothesis.h"
#include "../Command.h"
#include "../models/inc/Targets.h"
#include "../units/UnitObserver.h"

#include <boost/lexical_cast.hpp>

hammerQt::hammerQt(QWidget *parent)
    : QMainWindow(parent)
{
	ui.setupUi(this);

	//create the Delta3D QGLWidget
	mGLWidget = new dtQt::OSGAdapterWidget(this);
	ui.layout3d->addWidget(mGLWidget);

	//setup the map scene shown in the mapView
	mMap = new QGraphicsScene();
	mMap->setItemIndexMethod(QGraphicsScene::NoIndex);
	ui.mapView->setScene(mMap);

	//setup the results tab
	mResultsPlot = new QwtPlot(this);
	mResultsPlot->canvas()->setPaintAttribute(QwtPlotCanvas::PaintCached, false);
	mResultsPlot->canvas()->setPaintAttribute(QwtPlotCanvas::PaintPacked, false);
	mResultsPlot->canvas()->setAttribute(Qt::WA_PaintOnScreen, true);
	mResultsPlot->setAxisScale(QwtPlot::yLeft, -1, 1);
	//mResultsPlot->setAxisScale(QwtPlot::xBottom, 0, 60);
	mPlotResults = true;

	ui.resultsTab->addWidget(mResultsPlot);

	setCentralWidget(ui.centralWidget);

	//hide paused labels
	ui.redPausedLabel->setVisible(false);
	ui.bluePausedLabel->setVisible(false);

	//add formation actions to a group
	formationGroup = new QActionGroup(this);
	formationGroup->addAction(ui.action_None);
	formationGroup->addAction(ui.action_Line);
	formationGroup->addAction(ui.action_Column);
	formationGroup->addAction(ui.actionCi_rcle);
	formationGroup->addAction(ui.action_Wedge);

	manoeuvreGroup = new QActionGroup(this);
	manoeuvreGroup->addAction(ui.action_None_2);
	manoeuvreGroup->addAction(ui.action_Attack);
	manoeuvreGroup->addAction(ui.action_Retreat);
	manoeuvreGroup->addAction(ui.action_Defend);
	manoeuvreGroup->addAction(ui.action_Pincer);
	manoeuvreGroup->addAction(ui.action_Convoy);

	mPathLayer = new QGraphicsItemGroup();
	mMap->addItem(mPathLayer);

	mHypothesisLayer = new QGraphicsItemGroup();
	mMap->addItem(mHypothesisLayer);

	mGroupLayer = new QGraphicsItemGroup();
	mMap->addItem(mGroupLayer);

	mObservedGroupLayer = new QGraphicsItemGroup();
	mMap->addItem(mObservedGroupLayer);

	//hide the hypothesis stuff until we launch the hypotheses
	ui.hypothesesDockWidget->hide();
	ui.tabWidget->setTabEnabled(2, false);
	ui.action_Save_2->setEnabled(false);
	tabifyDockWidget(ui.dockWidget, ui.hypothesesDockWidget);

	//setup the hypothesis table
	ui.hypothesesTable->setColumnCount(2);
	//ui.hypothesesTable->verticalHeader()->hide();
	QStringList labels;
	labels << "Name" << "Confidence";
	ui.hypothesesTable->setHeaderLabels(labels);
}

hammerQt::~hammerQt()
{

}

void hammerQt::initialiseMap(std::string mapName) {
	LOG_DEBUG("Initialising the map: "+mapName);

	QString map = QString(mapName.c_str());

	mMap->setSceneRect(0, 0, 512, 512);
	//load map image from filename
	QGraphicsPixmapItem* bg = mMap->addPixmap(QPixmap(QString("images/%1.png").arg(map)));
	bg->setTransformationMode(Qt::SmoothTransformation);
	bg->setZValue(-1);
	//add map name
	mMap->addText(map);
}

void hammerQt::updateMap(std::vector<Unit*>& units) {
	//make all units invisible
	for (std::map<int,UnitMarker*>::iterator it = mUnitMarkers.begin(); it != mUnitMarkers.end(); it++) {
		it->second->setAlive(false);
	}
	//show and update all markers that are still alive
	for (unsigned int i=0; i < units.size(); i++) {
		std::map<int,UnitMarker*>::iterator marker = mUnitMarkers.find(units[i]->getUnitID());
		if (marker != mUnitMarkers.end() && (units[i]->getObservedPositionVariance() < 2000 || units[i]->getSide() == NONE)) {
			marker->second->setAlive(true);
			osg::Vec3 pos;
			if (units[i]->getSide() == NONE) {
				pos = units[i]->getPosition();
			} else {
				pos = units[i]->getObservedPosition();
				if (!units[i]->isObserved()) {
					marker->second->setAlive(false);
				}
			}
			QPointF p(coordToPoint(pos[0],pos[1]));
			marker->second->setPos(p);
			marker->second->setVariance(distToPixels(sqrt(units[i]->getObservedPositionVariance())));
			// dtUtil::Log::GetInstance().LogMessage(dtUtil::Log::LOG_INFO,__FUNCTION__, __LINE__,"Moved map marker for %s(%d), coord (%f, %f), pos (%f, %f)",units[i]->GetName().c_str(),units[i]->getUnitID(), pos.x(),pos.y(),p.x(),p.y());
		}
	}
	//leave dead units invisible (uncomment this [and comment first loop] if logging of dead units is important)
	/*for (std::map<int,UnitMarker*>::iterator it = mUnitMarkers.begin(); it != mUnitMarkers.end(); it++) {
		bool found = false;
		int item = 0;
		for (unsigned int i=0; i < units.size(); i++) {
			if (units[i]->mUnitID == it->first) {
				found = true;
				item = i;
				break;
			}
		}
		if (!found) {
			it->second->setAlive(false);
			dtUtil::Log::GetInstance().LogMessage(dtUtil::Log::LOG_INFO,__FUNCTION__, __LINE__,"Removed map marker for %s",units[item]->m_Name.c_str());
		}
	}*/

	//fade out the hypothesis paths
	for (std::map<int,QGraphicsItemGroup*>::iterator it = mHypothesisLayerItems.begin(); it != mHypothesisLayerItems.end(); it++) {
		if (it->second) {
			QList<QGraphicsItem*> items = it->second->childItems();
			foreach(QGraphicsItem* item, items) {
				QGraphicsPathItem* p = dynamic_cast<QGraphicsPathItem*>(item);
				if (p != NULL) {
					QPen pen = p->pen();
					QColor c = pen.color();
					if (c.alphaF() > 0.25) {
						c.setAlphaF(c.alphaF() - 0.002);
						pen.setColor(c);
						p->setPen(pen);
					}
				}
			}
		}
	}
}

void hammerQt::initialiseUnits(std::vector< Unit* > units) {
	mUnits = units;

	//add unit markers to map
	for (unsigned int i=0; i < units.size(); i++) {

		//dont show observers
		//UnitObserver* o = dynamic_cast<UnitObserver*>(units[i]);
		//if (o) continue;

		UnitMarker* unit = new UnitMarker(units[i]->getUnitID(), units[i]->getSide());
		mMap->addItem(unit);
		//QGraphicsItem* unit = mMap->addEllipse(-5, -5, 5, 5, QPen(), red);
		osg::Vec3 pos = units[i]->getPosition();
		QPointF p(coordToPoint(pos[0],pos[1]));
		unit->setPos(p);
		unit->setVariance(distToPixels(sqrt(units[i]->getObservedPositionVariance())));
		mUnitMarkers[units[i]->getUnitID()] = unit;
		dtUtil::Log::GetInstance().LogMessage(dtUtil::Log::LOG_INFO,__FUNCTION__, __LINE__,"Added map marker for %s, coord (%f, %f), pos (%f, %f)",units[i]->getName().c_str(), pos[0],pos[1],p.x(),p.y());
	}

	//add units to the unit table
	ui.unitsTable->setRowCount(units.size());
	ui.unitsTable->setColumnCount(2);
	ui.unitsTable->verticalHeader()->hide();
	QStringList labels;
	labels << "Name" << "Health";
	ui.unitsTable->setHorizontalHeaderLabels(labels);

	//UnitID is the row index
	for (unsigned int i=0; i < units.size(); i++) {
		//dont show observers
		//UnitObserver* o = dynamic_cast<UnitObserver*>(units[i]);
		//if (o) continue;

		QTableWidgetItem *nameItem = new QTableWidgetItem(tr("%1").arg(units[i]->getName().c_str()));
		ui.unitsTable->setItem(units[i]->getUnitID(), 0, nameItem);

		QProgressBar *healthBar = new QProgressBar();
		healthBar->setValue(units[i]->getHealth());
		//healthBar->setOrientation(Qt::Vertical);
		//healthBar->setTextVisible(false);
		ui.unitsTable->setCellWidget(units[i]->getUnitID(), 1, healthBar);

		if (units[i]->getSide() == RED) {
			QBrush red = QBrush(QColor(200,60,60));
			nameItem->setBackground(red);
		} else {
			QBrush blue = QBrush(QColor(100,150,255));
			nameItem->setBackground(blue);
		}

		/*UnitFiring *unit = dynamic_cast<UnitFiring*>(units[i].get());
		if (unit) {
			QTableWidgetItem *reloadItem = new QTableWidgetItem(tr("%1").arg(unit->getRemainingReloadTime()));
			ui.unitsTable->setItem(units[i]->getUnitID(), 2, reloadItem);
		}*/

		ui.unitsTable->setRowHeight(units[i]->getUnitID(), 20);
		ui.unitsTable->setColumnWidth(1,80);
	}
}

void hammerQt::removeUnit(Unit *unit) {
	ui.unitsTable->removeCellWidget(unit->getUnitID(),1);
	QTableWidgetItem *nameItem = ui.unitsTable->item(unit->getUnitID(),0);
	QTableWidgetItem *deadItem = new QTableWidgetItem("");
	if (unit->getSide() == RED) {
		QBrush red(QColor(200,60,60,100));
		nameItem->setBackground(red);
		deadItem->setBackground(red);
	} else {
		QBrush blue(QColor(100,150,255,100));
		nameItem->setBackground(blue);
		deadItem->setBackground(blue);
	}
	ui.unitsTable->setItem(unit->getUnitID(),1,deadItem);
}

void hammerQt::clearUnits() {
	for (std::map<int,UnitMarker*>::iterator it = mUnitMarkers.begin(); it != mUnitMarkers.end(); it++) {
		delete it->second;
	}
	mUnitMarkers.clear();
	for (int i=ui.unitsTable->rowCount()-1; i >= 0; i--) {
		ui.unitsTable->removeRow(i);
	}
}


void hammerQt::updateUnits(std::vector<Unit*> units) {
	//mUnits = units;

	updateMap(units);
	updateUnitsTable(units);
}

void hammerQt::updateUnitsTable(std::vector<Unit*>& units) {
	for (unsigned int i=0; i < units.size(); i++) {
		QProgressBar *healthBar = dynamic_cast<QProgressBar*>(ui.unitsTable->cellWidget(units[i]->getUnitID(),1));
		if (healthBar) healthBar->setValue(units[i]->getHealth());
	}
}

void hammerQt::updateUnitGroups(std::vector<UnitGroup*> groups) {
	//remove existing group markers
	for (unsigned int i=0; i < mUnitGroupMarkers.size(); i++) {
		delete mUnitGroupMarkers[i];
	}
	mUnitGroupMarkers.clear();
	for (unsigned int i=0; i < groups.size(); i++) {
		//create group marker
		GroupMarker *m = new GroupMarker(groups[i], GroupMarker::GROUP);
		osg::Vec3 p = groups[i]->getCentre();
		m->setPos(coordToPoint(p.x(), p.y()));
		mUnitGroupMarkers.push_back(m);
		mGroupLayer->addToGroup(m);
	}
}

void hammerQt::updateObservedUnitGroups(std::vector<UnitGroup*> groups) {
	//remove existing group markers
	for (unsigned int i=0; i < mObservedUnitGroupMarkers.size(); i++) {
		delete mObservedUnitGroupMarkers[i];
	}
	mObservedUnitGroupMarkers.clear();
	for (unsigned int i=0; i < groups.size(); i++) {
		//create group marker
		GroupMarker *m = new GroupMarker(groups[i], GroupMarker::OBSERVED_GROUP);
		osg::Vec3 p = groups[i]->getCentre();
		//TODO this shouldn't always be updated...
		// should be last observed position
		// variance should be last observed variance + increasing variance with time
		m->setPos(coordToPoint(p.x(), p.y()));
		mObservedUnitGroupMarkers.push_back(m);
		mObservedGroupLayer->addToGroup(m);
	}
}

void hammerQt::updateMapParams(std::vector<osg::Vec3> params) {
	LOG_INFO("New Command Set Started");
	for (std::vector<osg::Vec3>::iterator it = params.begin(); it != params.end(); it++) {
		//osg::Vec3 p = theApp->getTargets()->getUnitGroups(RED)[0]->getUnits()[0]->getPosition();
		//QPointF imagePoint = coordToPoint(it->x()+p[0], it->y()+p[1]);
		QPointF imagePoint = coordToPoint(it->x(), it->y());
		(*it)[0] = imagePoint.x();
		(*it)[1] = imagePoint.y();
	}
	for (std::map<int,UnitMarker*>::iterator it = mUnitMarkers.begin(); it != mUnitMarkers.end(); it++) {
		if (it->second->getSide() == RED) it->second->setParams(params);
	}
}

void hammerQt::updateClusters(std::vector<std::vector<int> > clusters) {
	//reset clusters
	for (std::map<int,UnitMarker*>::iterator it = mUnitMarkers.begin(); it != mUnitMarkers.end(); it++) {
		it->second->setCluster(-1);
		it->second->setId(-1);
	}
	//set cluster for those units that have been clustered
	for(unsigned int i=0; i < clusters.size(); i++) {
		for(unsigned int j=0; j < clusters[i].size(); j++) {
			mUnitMarkers[clusters[i][j]]->setCluster(i);
			//mUnitMarkers[clusters[i][j]]->setId(clusters[i][j]);
			mUnitMarkers[clusters[i][j]]->setId(i);
		}
	}
}

void hammerQt::clearPaths() {
	//delete paths
	for (std::vector<QGraphicsItem*>::iterator it = mPaths.begin(); it != mPaths.end(); it++) {
		delete *it;
	}
	mPaths.clear();
}

void hammerQt::addPath(std::vector<osg::Vec2> path) {
	QGraphicsItem *lineItem = createPath(path);
	mPaths.push_back(lineItem);
	mPathLayer->addToGroup(lineItem);
}

QGraphicsPathItem *hammerQt::createPath(std::vector<osg::Vec2>& path) {
	int mapWidth = theApp->getTerrain()->GetHeightField()->GetNumColumns();
	float imageSize = 512.0;
	float mult = imageSize / mapWidth;
	QPainterPath* line = new QPainterPath();
	if (!path.empty()) {
		std::vector<osg::Vec2>::iterator it = path.begin();
		line->moveTo(it->x()*mult,imageSize-(it->y()*mult));
		for (; it != path.end(); it++) {
			line->lineTo(it->x()*mult,imageSize-(it->y()*mult));
		}
	}
	QGraphicsPathItem *lineItem = new QGraphicsPathItem(*line);
	return lineItem;
}

void hammerQt::addHypothesis(Hypothesis* h) {
	/*ui.hypothesesTable->setRowCount(ui.hypothesesTable->rowCount() + 1);
	QTableWidgetItem *nameItem = new QTableWidgetItem(tr("%1").arg(h->getName().c_str()));
	ui.hypothesesTable->setItem(h->getID(), 0, nameItem);*/

	QTreeWidgetItem *hypothesisItem = new QTreeWidgetItem((QTreeWidget*) 0, QStringList(QString("%1").arg(h->getName().c_str())));
	QList<QTreeWidgetItem *> items;
	std::vector<Command::CommandType> commands = h->getAllCommands();
	//create curves
	int hypothesisIndex = ui.hypothesesTable->topLevelItemCount();
	std::map<int, ResultsPlot*>::iterator curvesIt = mResultsPlotCurves.find(hypothesisIndex);
	if (curvesIt == mResultsPlotCurves.end()) {
		//create curves
		ResultsPlot *c = new ResultsPlot(mResultsPlot, commands.size());
		mResultsPlotCurves[hypothesisIndex] = c;
		LOG_INFO("Creating "+boost::lexical_cast<std::string>(commands.size())+" results plot curves")
	}
	//create command sub-tree
	for (unsigned int i = 0; i < commands.size(); ++i) {
		QTreeWidgetItem* command = new QTreeWidgetItem(hypothesisItem, QStringList(QString("Command: %1").arg(i)));
		command->setBackground(0,QBrush(mResultsPlotCurves[hypothesisIndex]->getCurve(i)->pen().color()));
		items.append(command);
	}
	ui.hypothesesTable->addTopLevelItem(hypothesisItem);

	TargetHypothesis* th = dynamic_cast<TargetHypothesis*>(h);
	if (th) {
		connect(th, SIGNAL(hypothesisClusterUpdate(std::vector<std::vector<int> >)), this, SLOT(updateClusters(std::vector<std::vector<int> >)));
	}
}

void hammerQt::showHypothesisResults(int id, std::vector<HypothesisResult*> r) {
	if (r.size() > 0) {
		//remove previous results for this hypothesis
		delete mHypothesisLayerItems[id];

		HypothesisResult::PointsType type = r[0]->getPointsType();

		//create parent object
		if (type == HypothesisResult::ABSOLUTE || type == HypothesisResult::WORLD) {
			mHypothesisLayerItems[id] = new QGraphicsItemGroup(mHypothesisLayer);
		} else if (type == HypothesisResult::RELATIVE) {
			//create parent object
			//TODO find marker for r->getStartUnits() for the parent
			//TODO unfortunately new group markers are created each frame
			//mHypothesisLayerItems[id] = new QGraphicsItemGroup();
		}

		//show results of each command
		for (unsigned int i=0; i < r.size(); i++) {
			//draw path
			std::vector<osg::Vec2> path = r[i]->getPoints();
			if (!path.empty() && type == HypothesisResult::WORLD) {
				//convert WORLD path to map coords
				for (std::vector<osg::Vec2>::iterator it = path.begin(); it != path.end(); it++) {
					*it = coordToPoint(*it);
				}
			}
			if (!path.empty()) {
				QGraphicsPathItem *p = createPath(path);
				p->setParentItem(mHypothesisLayerItems[id]);
				QPen pen(QColor(Qt::red));
				//make pen width depend on confidence
				float conf = (r[i]->getConfidence()+0.1) * 2;
				if (conf < 0.2) conf = 0;
				pen.setWidthF(conf);
				p->setPen(pen);
				//add a label at the mid point
				osg::Vec2 mid = path[(int)path.size()/3];
				if (path.size() == 2) {
					mid = path[0] + ((path[1] - path[0])/3);
				}
				QGraphicsTextItem* t = new QGraphicsTextItem(mHypothesisLayerItems[id]);
				int mapWidth = theApp->getTerrain()->GetHeightField()->GetNumColumns();
				float imageSize = 512.0;
				float mult = imageSize / mapWidth;
				t->setPos(mid.x()*mult, 512-(mid.y()*mult));
				QFont f = t->font();
				f.setPointSize(3);
				t->setFont(f);
				t->setPlainText(QString::fromStdString(r[i]->getLabel()) + QString(": %1").arg(r[i]->getConfidence()));
			} else {
				/*UnitGroup* g = r[i]->getTargetUnits().get();
				std::vector<int> u = g->getUnitIDs();
				for (unsigned int j = 0; j < u.size(); j++) {
					mUnitMarkers[u[j]]->setCluster(i);
				}*/
			}
		}
	}
}

/**
 * Called from HypothesisManager::update()
 */
void hammerQt::updateResults(std::vector<dtCore::RefPtr<Hypothesis> > hypotheses) {
	/*for (unsigned int i=0; i < mGroupDeviations.size(); i++) {
		delete mGroupDeviations[i];
	}
	mGroupDeviations.clear();*
	for (unsigned int i=0; i < hypotheses.size(); i++) {
		osg::Vec3 pos = hypotheses[i]->getObservationGroup()->getCentre();
		float deviation = hypotheses[i]->getDeviation(theApp->getTerrainMap()->worldToMapCoords(osg::Vec2(pos.x(), pos.y())));
		if (deviation != -1) {
			//TODO update group graphic
			//std::cout << deviation << std::endl;
		}
	}*/

	if (mPlotResults) {
		for (unsigned int i=0; i < hypotheses.size(); i++) {
			//create curves to plot results (should have been created in addHypothesis)
			/*std::map<int, ResultsPlot*>::iterator curvesIt = mResultsPlotCurves.find(i);
			std::vector<float> confidences = hypotheses[i]->getConfidences();
			if (curvesIt == mResultsPlotCurves.end()) {
				//create curves
				ResultsPlot *c = new ResultsPlot(mResultsPlot, confidences.size());
				mResultsPlotCurves[i] = c;
				LOG_INFO("Creating "+boost::lexical_cast<std::string>(confidences.size())+" results plot curves")
			}*/
			//ResultsPlot *curves = mResultsPlotCurves[i];

			//for (unsigned int j=0; j < confidences.size(); j++) {
				//LOG_INFO("Adding result "+boost::lexical_cast<std::string>(confidences[j]))
				//curves->addResult(j, theApp->getSimTime(), confidences[j]);
			//}
		}
		// finally, refresh the plot
		//mResultsPlot->replot();
	}
}

float hammerQt::distToPixels(float dist) {
	float mapHorizontalSize = theApp->getTerrain()->GetHorizontalSize();//10000.0;
	float imageSize = 512.0;
	return (dist / mapHorizontalSize) * imageSize;
}

QPointF hammerQt::coordToPoint(float x, float y) {
	//converts the coordinate system used in the simulator to map image coordinates
	float mapHorizontalSize = theApp->getTerrain()->GetHorizontalSize();
	float imageSize = 512.0;
	return QPointF((x / mapHorizontalSize + 0.5) * imageSize, (-y / mapHorizontalSize + 0.5) * imageSize);
}

osg::Vec2 hammerQt::coordToPoint(osg::Vec2 c) {
	//converts the coordinate system used in the simulator to map image coordinates
	QPointF p = coordToPoint(c.x(), -c.y());
	return osg::Vec2(p.x(), p.y());
}

void hammerQt::on_zoomSlider_valueChanged(int value) {
	float scale = (value/10.0)+1;
	QMatrix m(scale, 0, 0, scale, 0, 0);
	ui.mapView->setMatrix(m);
}

void hammerQt::on_action_Replay_triggered() {
	QString fileName = QFileDialog::getOpenFileName(this,
	     tr("Open Log File"), "results/logs", tr("Log Files (*.txt)"));
	theApp->loadReplay(fileName.toStdString());
}

void hammerQt::on_actionExit_triggered() {
	close();
}

void hammerQt::on_actionDemonstration_triggered() {
        theApp->getPlanCombiner()->CombinePlansDemo();
}

void hammerQt::on_action_Fullscreen_triggered(bool checked) {
	if (checked)
		showFullScreen();
	else
		showNormal();
}

void hammerQt::on_unitsTable_itemClicked(QTableWidgetItem * item ) {
	emit unitSelected(ui.unitsTable->item(item->row(),0)->text());
}

void hammerQt::on_plotCheckBox_stateChanged(int state) {
	mPlotResults = !mPlotResults;
}

void hammerQt::setHitPos(osg::Vec3 p) {
	std::ostringstream oss;
	oss << p;
	statusBar()->showMessage("Pos: " + QString(oss.str().c_str()),2000);
}

void hammerQt::ShowPausedRed(bool val) {
	ui.redPausedLabel->setVisible(val);
}

void hammerQt::ShowPausedBlue(bool val) {
	ui.bluePausedLabel->setVisible(val);
}

void hammerQt::ShowGameOverMessage(std::string message) {


	int ret = QMessageBox::information(this, QString("Game Over"),
			QString((message + ". Restart?").c_str()), QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok);
	if (ret == QMessageBox::Ok) {
		theApp->restartScenario();
	}
}

void hammerQt::SetTime( signed int total_sec, signed int goal_sec ) {
	signed int total_min = floor(total_sec/60);
	signed int goal_min = floor(goal_sec/60);

	ui.timeRemainingLabel->setText( QString::number(total_min) + "m " + QString::number(total_sec-60*total_min) + "s");
	ui.timeGoalLabel->setText( QString::number(goal_min) + "m " + QString::number(goal_sec-60*goal_min) + "s");
}

void hammerQt::SetTimeScale(double timeScale, double desiredTimeScale, double requestedTimeScale) {
	std::ostringstream oss;

    oss << std::fixed << std::setprecision(2) << timeScale << "x";

    if (desiredTimeScale > 0 && desiredTimeScale != timeScale) {
		oss << " (" << std::fixed << std::setprecision(2) << desiredTimeScale
				<< "x desired)";
	}
	if (requestedTimeScale > 0 && requestedTimeScale != timeScale) {
		oss << " (" << std::fixed << std::setprecision(2) << requestedTimeScale
				<< "x requested)";
	}

    ui.timeScaleLabel->setText(QString(oss.str().c_str()));
}
