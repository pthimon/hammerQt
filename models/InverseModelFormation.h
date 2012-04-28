#ifndef INVERSEMODELFORMATION_H_
#define INVERSEMODELFORMATION_H_

#include "inc/InverseModel.h"
#include "../units/Unit.h"

class InverseModelFormation;

class FormationConnection {
public:
	FormationConnection(InverseModelFormation* grp, float dist, int strength):mGroup(grp),mDist(dist),mStrength(strength) {
		mStart = NULL;
		mEnd = NULL;
	}
	FormationConnection(float dist, int strength, Unit *u1, Unit *u2):
		mDist(dist),
		mStrength(strength),
		mStart(u1),
		mEnd(u2) {}
	void addConnection(Unit *u) {
		if (mStart == NULL) {
			mStart = u;
		} else {
			mEnd = u;
		}
	}
	//gets the connected unit that isn't u
	Unit *getConnectedUnit(Unit *u) {
		if (u == mStart) {
			return mEnd;
		} else {
			return mStart;
		}
	}
	InverseModelFormation *getGroup() { return mGroup; }
	float getDist() { return mDist; }
	int getStrength() { return mStrength; }
private:
	InverseModelFormation *mGroup;
	float mDist;
	int mStrength;
	Unit *mStart;
	Unit *mEnd;
};

class InverseModelFormation : public InverseModel
{
public:
	enum UnitFormation {
		NONE,
		CIRCLE,
		WEDGE,
		LINE,
		COLUMN
	};

	InverseModelFormation();
	InverseModelFormation(std::vector<Unit*>& units, UnitFormation form = NONE, bool autoLeader = true, bool autoArrange = true);
	virtual ~InverseModelFormation();

	virtual void update();
	virtual void addUnit(Unit *unit) {};
	virtual void removeUnit(const Unit *unit, bool clear);

	void setFormation(UnitFormation formation);
	osg::Vec3 getAveragePosition();
	osg::Vec3 getAverageHeading();
	int getNearestUnitTo(osg::Vec3 pos);
	int getNearestUnconnectedUnitTo(Unit* unit);
	Unit* getLeader();
	void addFormationConnection(FormationConnection *fc, Unit *u1, Unit *u2);
	int getMiddleUnit();

/*	void addFormationConnection(FormationConnection *fc);
	void removeAllFormationConnections(const UnitGroup* grp = NULL);
	bool isConnected() { return !mFormationConnections.empty(); }
	int getUnconnectedNeighbour(std::vector<Unit*>&);
	void setFormationLeader() { mLeader = true; }
	bool isFormationLeader() { return mLeader; }
	std::string getFormation() { if (mLeader) return mFormation; else return NULL; }

	std::vector<FormationConnection*> mFormationConnections;
	bool mLeader;
	std::string mFormation;
	*/

private:
	std::vector<Unit*> mUnits;
	UnitFormation mFormation;
	std::vector<int> mOrdering;
	std::map<Unit*,std::vector<FormationConnection*> > mUnitConnections;
	bool mAutoLeader;
	bool mAutoArrange;
};

#endif /*INVERSEMODELFORMATION_H_*/
