#include "InverseModelFormation.h"
#include "../common.h"
#include "../HTAppBase.h"

InverseModelFormation::InverseModelFormation(): mFormation(InverseModelFormation::NONE)
{
}

InverseModelFormation::InverseModelFormation(std::vector<Unit*>& units, UnitFormation form, bool autoLeader, bool autoArrange): mUnits(units), mFormation(form), mAutoLeader(autoLeader), mAutoArrange(autoArrange)
{
	mAutoArrange = false;
	setFormation(form);
}

InverseModelFormation::~InverseModelFormation()
{
}

void InverseModelFormation::update() {
	for (unsigned int i=0; i < mUnits.size(); i++) {
		//if the unit is in a formation, then update its goal position based on the connected units positions
		if (mUnitConnections[mUnits.at(i)].size() > 0 && mUnits.at(i) != getLeader()) {
			//TODO obstacle avoidance (test gradient of update point, if too steep, A* plan around it)
			//we're in a formation so update goal position
			osg::Vec3 offset = osg::Vec3(0,0,0);
			std::vector<FormationConnection*> formationConnections = mUnitConnections[mUnits.at(i)];
			for (unsigned int j=0; j < formationConnections.size(); j++) {
				//vector to connected unit
				osg::Vec3 dir = (formationConnections.at(j)->getConnectedUnit(mUnits.at(i))->getPosition() - mUnits.at(i)->getPosition()) * formationConnections.at(j)->getStrength();
				//keep a certain distance away
				dir *= dir.normalize() - formationConnections.at(j)->getDist();
				offset += dir;
			}
			osg::Vec3 pos = mUnits.at(i)->getPosition();
			//gradient to 2 blocks ahead
			osg::Vec3 lookahead = offset;
			lookahead.normalize();
			lookahead *= (theApp->getTerrainMap()->getNodeWidth() * 2);
			float gradient = (theApp->mapPointToTerrain(pos + lookahead) - pos).z();
			//std::cout << gradient << std::endl;
			if (fabs(gradient) > 50) {
				//decide which way to turn (find direction of leader)
				osg::Vec3 dirLeader = getLeader()->getPosition() - pos;
				//dir rotated by 90 degrees
				osg::Vec2 v90 = osg::Vec2(-dirLeader[1], dirLeader[0]);
				//find out whether angle is to the left or right
				float beta = osg::Vec2(pos.x(),pos.y()) * v90;
				if (beta > 0) {
					//rotate left
					lookahead = osg::Vec3(lookahead[1], -lookahead[0], 0);
				} else {
					//rotate right
					lookahead = osg::Vec3(-lookahead[1], lookahead[0], 0);
				}
				mUnits.at(i)->setGoalPosition(theApp->mapPointToTerrain(pos + lookahead), true);
			} else {
				mUnits.at(i)->setGoalPosition(theApp->mapPointToTerrain(pos + offset), false);
			}
		}
	}
}

void InverseModelFormation::removeUnit(const Unit *unit, bool clear) {
	bool found = false;
	std::vector<Unit*>::iterator it;
	for (it = mUnits.begin(); it != mUnits.end(); it++) {
		if (*it == unit) {
			found = true;
			mUnits.erase(it);
		}
	}

	if (!found) return;

	// re-setFormation
	if (!clear) setFormation(mFormation);
}

void InverseModelFormation::setFormation(UnitFormation formation) {

	if (mUnits.size() < 2) return;

	//remove existing formations
	mUnitConnections.clear();

	mFormation = formation;
	mOrdering.clear();

	if (mAutoArrange) {
		//assign leader
		if (mAutoLeader) {
			mOrdering.push_back(getNearestUnitTo(getAveragePosition()));
		} else {
			mOrdering.push_back(0);
		}
		//arranging done within switch below
	} else {
		//assign complete ordering
		if (mAutoLeader) {
			unsigned int leader = getMiddleUnit();
			mOrdering.push_back(leader);
			for (unsigned int i=0; i < mUnits.size(); i++) {
				if (i != leader) mOrdering.push_back(i);
			}
		} else {
			for (unsigned int i=0; i < mUnits.size(); i++) {
				mOrdering.push_back(i);
			}
		}
	}

	//TODO deal with even number of units in wedge or line
	switch (mFormation) {
		case CIRCLE:
			for (unsigned int i=0; i < mUnits.size()-1; i++) {
				//connect with all other units (that aren't already connected to you
				for (unsigned int j=i+1; j < mUnits.size(); j++) {
					if (i==0 && mAutoArrange) {
						if (j==1)
							mOrdering.push_back(getNearestUnconnectedUnitTo(mUnits.at(mOrdering.at(0))));
						else
							mOrdering.push_back(getNearestUnconnectedUnitTo(mUnits.at(mOrdering.at(j-2))));
					}
					FormationConnection *fc = new FormationConnection((InverseModelFormation *)this, 50, 1);
					addFormationConnection(fc, mUnits.at(mOrdering.at(i)), mUnits.at(mOrdering.at(j)));
				}
			}
			break;
		case WEDGE:
			for (unsigned int i=0; i < mUnits.size()-1; i++) {
				//connect in triangle formation
				int k = 1;
				for (unsigned int j=i+1; (j < mUnits.size() && k < 3); j++) {
					float weight = 0;
					if (k == 1) {
						if ((i % 2) == 1) {
							weight = 1.5 * (j/2);
						} else if (i == 0) {
							weight = 1;
						} else {
							//no connection
							k++;
							continue;
						}
					} else {
						weight = 1;
					}
					//std::cout << i << " to " << j << ": " << weight << std::endl;
					try {
						mOrdering.at(j);
					} catch (...) {
						if (j==1 && mAutoArrange)
							mOrdering.push_back(getNearestUnconnectedUnitTo(mUnits.at(mOrdering.at(0))));
						else
							mOrdering.push_back(getNearestUnconnectedUnitTo(mUnits.at(mOrdering.at(j-2))));
					}
					FormationConnection *fc = new FormationConnection((InverseModelFormation *)this, 50*(weight), 1);
					addFormationConnection(fc, mUnits.at(mOrdering.at(i)), mUnits.at(mOrdering.at(j)));
					k++;
				}
			}
			break;
		case LINE:
			for (unsigned int i=0; i < mUnits.size()-1; i++) {
				//connect with all other units (that aren't already connected to you with increasing weights
				int k = 1;
				for (unsigned int j=i+1; j < mUnits.size(); j++) {
					int weight = 0;
					if ((k % 2) == 0) {
						//even
						weight = k / 2;
					} else {
						//odd
						weight = i + ((k+1)/2);
					}
					if (k==1 && (i%2)==1) weight++;

					if (i==0 && mAutoArrange) {
						if (j==1)
							mOrdering.push_back(getNearestUnconnectedUnitTo(mUnits.at(mOrdering.at(0))));
						else
							mOrdering.push_back(getNearestUnconnectedUnitTo(mUnits.at(mOrdering.at(j-2))));
					}

					FormationConnection *fc = new FormationConnection((InverseModelFormation *)this, 10*(weight), 1);
					addFormationConnection(fc, mUnits.at(mOrdering.at(i)), mUnits.at(mOrdering.at(j)));
					k++;
				}
			}
			break;
		case COLUMN:
			//leader must be pre-assigned to mOrdering.at(0)
			for (unsigned int i=0; i < mUnits.size()-1; i++) {
				if (mAutoArrange) mOrdering.push_back(getNearestUnconnectedUnitTo(mUnits.at(mOrdering.at(i))));

				FormationConnection *fc = new FormationConnection((InverseModelFormation *)this, 50, 1);
				addFormationConnection(fc, mUnits.at(mOrdering.at(i)), mUnits.at(mOrdering.at(i+1)));
			}
			break;
		default:
			std::cout << "WARN: Unknown formation: " << formation << std::endl;
			return;
	}
}

osg::Vec3 InverseModelFormation::getAveragePosition() {
	osg::Vec3 avPos = osg::Vec3(0,0,0);
	for (unsigned int i=0; i < mUnits.size(); i++) {
		avPos += mUnits.at(i)->getPosition();
	}
	avPos /= mUnits.size();
	return avPos;
}

osg::Vec3 InverseModelFormation::getAverageHeading() {
	osg::Vec3 avHeading = osg::Vec3(0,0,0);
	for (unsigned int i=0; i < mUnits.size(); i++) {
		avHeading += mUnits.at(i)->getPosition();
	}
	avHeading /= mUnits.size();
	return avHeading;
}

int InverseModelFormation::getNearestUnitTo(osg::Vec3 pos) {
	//find closest unit to pos
	float dist = INFINITY;
	int closest = -1;
	for (unsigned int i=0; i < mUnits.size(); i++) {
		double d = (mUnits.at(i)->getPosition() - pos).length();
		if (d < dist) {
			dist = d;
			closest = i;
		}
	}
	return closest;
}

int InverseModelFormation::getNearestUnconnectedUnitTo(Unit* unit) {
	//find closest unconnected unit to 'unit'
	double dist = INFINITY;
	int closest = -1;
	for (unsigned int i=0; i < mUnits.size(); i++) {
		if (mUnitConnections[mUnits.at(i)].size() == 0 && mUnits.at(i) != unit) {
			double d = (mUnits.at(i)->getPosition() - unit->getPosition()).length();
			if (d < dist) {
				dist = d;
				closest = i;
			}
		}
	}
	return closest;
}

Unit* InverseModelFormation::getLeader() {
	return mUnits.at(mOrdering.at(0));
}


void InverseModelFormation::addFormationConnection(FormationConnection *fc, Unit *u1, Unit *u2) {
	fc->addConnection(u1);
	fc->addConnection(u2);
	mUnitConnections[u1].push_back(fc);
	mUnitConnections[u2].push_back(fc);
}

int InverseModelFormation::getMiddleUnit() {
	return floor(mUnits.size()/2);
}
