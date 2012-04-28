#pragma GCC diagnostic ignored "-Wextra"

#include "../common.h"

#include <dtUtil/xercesparser.h>
#include <dtUtil/xercesutils.h>
#include <xercesc/util/XMLString.hpp>
#include <dtUtil/mathdefines.h>
#include <dtCore/shadermanager.h>
#include <dtCore/shaderparameter.h>
#include <dtCore/transform.h>
#include <osg/ShapeDrawable>
#include <osg/Material>
#include <osg/PolygonOffset>
#include <osg/BoundingBox>

#include <cfloat>
#include <sstream>

#pragma GCC diagnostic warning "-Wextra"

#include "Unit.h"
#include "../HTapp.h"
#include "../eventlog.h"
#include "UnitConfigHandler.h"
#include "../terrain/terrain.h"

Unit::Unit(const std::string& name, osg::Vec3 position, double facing, UnitSide side,
		   bool controlled, bool remote, bool flying, int client, int id)
: dtCore::Object(name),
  m_Side(side),
  m_IsFlying(flying),
  m_IsControlled(controlled),
  m_IsRemote(remote),
  m_Initialized(false),
  m_StartHeading(facing),
  m_Health(100),
  m_StartPosition(position),
  mArmour(1),
  mNextWaypoint(0),
  mClientID(client),
  mUnitID(id),
  mPredictedUnit(false),
  mObservedPosition(osg::Vec3(0,0,0)),
  mObservedPositionVariance(std::numeric_limits<double>::max()),
  mObservedPositionTime(0),
  //assume we have an observer at an altitude of 7071m to cover the entire map,
  //the max distance based on the resolution of 30 is approx 250m.
  mObMaxDistance(70)
  {
  	m_Marker = 0;
  	//m_GoalPosition.set(NAN, NAN, NAN);
  	mShaderTextureCycleTime = dtUtil::RandFloat(2.0, 4.0);
	mShaderMoveXCycleTime = dtUtil::RandFloat(5.0, 8.0);
	mShaderMoveYCycleTime = dtUtil::RandFloat(5.0, 8.0);
	mShaderMoveZCycleTime = dtUtil::RandFloat(5.0, 8.0);

    //why isnt this done in dtCore::Object? Crazy.
    setName(name);

    mPath = new PathInterpolation(20, 5);

    mRoadMask = 1l << theApp->getTerrain()->GetLayerIndex("road");
    mForestMask =  1l << theApp->getTerrain()->GetLayerIndex("forest");

    mObMaxVariance = mObMaxDistance * mObMaxDistance;

    //record the unit name so it can still be read from HTAppBase when the unit dies
    theApp->setUnitName(id, name);
  }

Unit::~Unit()
{
	if (m_Initialized) {
		theApp->displayUnitHealth(this, INFINITY);

		for (unsigned int i=0; i < mWaypointMarkers.size(); i++) {
			theApp->getTerrain()->removeTerrainOverlayChild(mWaypointMarkers[i].get());
		}
		mWaypointMarkers.clear();

		if (!theApp->isHeadless()) theApp->getTerrain()->removeTerrainOverlayChild(mUnitMarker.get());
	}
}

Unit::Unit(Unit& u) :
	dtCore::Object(u.GetName()),
	m_Side(u.m_Side),
	m_IsFlying(u.m_IsFlying),
	m_IsControlled(u.m_IsControlled),
	m_IsRemote(u.m_IsRemote),
	m_Initialized(false),
	m_UnitType(u.m_UnitType),
	m_SensorRange(u.m_SensorRange),
	m_Visibility(u.m_Visibility),
	m_StartHeading(u.getHeading()),
	m_Health(u.m_Health),
	m_StartPosition(u.getPosition()),
	mArmour(u.mArmour),
	mNextWaypoint(0),
	mClientID(u.mClientID),
	mUnitID(u.mUnitID),
	mPredictedUnit(true)
{
	m_Marker = 0;
	//defaults
	mShaderTextureCycleTime = u.mShaderTextureCycleTime;
	mShaderMoveXCycleTime = u.mShaderMoveXCycleTime;
	mShaderMoveYCycleTime = u.mShaderMoveYCycleTime;
	mShaderMoveZCycleTime = u.mShaderMoveZCycleTime;

	setName(u.GetName());

	mPath = new PathInterpolation(20, 5);

	mRoadMask = u.mRoadMask;
	mForestMask =  u.mForestMask;
}

Unit* Unit::clone() {
	return new Unit(*this);
}

void Unit::init()
{
	setSelected(false);

	LoadMeshes();
	applyShader();
	addUnitMarker();
	initPhysics();

	m_Initialized = true;

    setGoalPosition(m_StartPosition);
	setPosition(m_StartPosition);
	setHeading(m_StartHeading);
}

bool Unit::parseUnitConfigFile(const std::string& file)
{
	std::string filePath;
	//get path from filename
	if (!file.empty()) {
		filePath = dtCore::FindFileInPathList( file );
		if (filePath.empty()) {
		     LOG_WARNING("Could not find unit config file to load.  Missing file name is """ + file + """" );
		}
	}

 	//parse
   UnitConfigHandler handler;
   dtUtil::XercesParser parser;
   bool parsed_well = parser.Parse(filePath, handler);
   if (!parsed_well) {
   		LOG_WARNING("Could not parse unit XML file: "+ filePath);
   }

   bool applied_well = applyConfigData(handler.mConfigData);
   if (!applied_well) {
   		LOG_WARNING("Could not apply unit XML file: "+ filePath);
   }

   return( applied_well || parsed_well );
}

/**
 * Override this function to first call applyUnitConfigData,
 * then call your own applyUnit*ConfigData
 */
bool Unit::applyConfigData(const UnitConfigData& data) {
	return applyUnitConfigData(data);
}

bool Unit::applyUnitConfigData(const UnitConfigData& data) {
	//do unit stuff
	m_UnitType = data.unit_name;
	m_IsFlying = data.flying;
	mArmour = data.armour;
	m_SensorRange = data.sensors.range;
        m_Visibility = data.sensors.visibility;

	return true;
}

void Unit::update()
{
	updateShaderParams();
	updatePhysics();

	// broadcast registered unit parts for networking
	std::map<dtCore::Transformable*, std::string>::iterator child_it;
	unsigned int i = 0;
	for (child_it = m_NetworkingChildrenMeshes.begin();
		 child_it != m_NetworkingChildrenMeshes.end(); child_it++)
	{
	    osg::Vec3 xyz;
   		osg::Vec3 hpr;
		dtCore::Transform current_tr;
		std::ostringstream oss;
		oss << getName() << i;
		child_it->first->GetTransform(current_tr);
		current_tr.GetTranslation(xyz);
		current_tr.GetRotation(hpr);
		theApp->SendPosition(oss.str(), xyz, hpr);
		i++;
	}
	//log the units position and heading (once every interval)
	theApp->BroadcastEvent(Event::POSITION_AND_HEADING, this);

	//store last position and time
	mLastPosition = mLastPositionTemp;
	mLastPositionTime = mLastPositionTimeTemp;
	mLastPositionTemp = getPosition();
	mLastPositionTimeTemp = theApp->getSimTime();

	//update unit marker
	if (mUnitMarker != 0) {
		dtCore::Transform trans;
		mUnitMarker->GetTransform(trans);
		trans.SetTranslation(getPosition());
		mUnitMarker->SetTransform(trans);
	}

	//update waypoints
	if (mPath != 0) {
		unsigned int waypoint = mPath->getWaypointIndex();
		while (waypoint > mNextWaypoint && waypoint < mWaypointMarkers.size()) {
			theApp->getTerrain()->removeTerrainOverlayChild(mWaypointMarkers.at(mNextWaypoint).get());
			mNextWaypoint++;
		}
		updateWaypointMarker();
	}
}

bool Unit::addGoalPosition(osg::Vec3 goal)
{
	if (!mPath) mPath = new PathInterpolation();

	addWaypointMarker(mPath->getFinalWaypoint(), goal);
	mPath->addWaypoint(goal);

	theApp->BroadcastEvent(Event::NEW_GOAL, this);

	return true;
}

bool Unit::setGoalPosition(osg::Vec3 goal, bool marker)
{
	if (m_IsFlying) {
		double h;
		//get goal altitude
		theApp->GetTerrainHeight(goal.x(),goal.y(),h);
		double goalAltitude = goal.z() - h;
		//if goal altitude is on the ground then set goal altitude to be in the air
		if (goalAltitude < 1) {
			//get current altitude
			osg::Vec3 pos = getPosition();
			theApp->GetTerrainHeight(pos.x(),pos.y(),h);
			double altitude = pos.z()-h;
			//set goal position to this altitude
			goal.z() += altitude;
		}
	}
	if (goal == getGoalPosition()) return false;

	if (!mPath) mPath = new PathInterpolation();

	//remove markers
	for (unsigned int i = 0; i < mWaypointMarkers.size(); i++) {
		theApp->getTerrain()->removeTerrainOverlayChild(mWaypointMarkers[i].get());
	}
	mWaypointMarkers.clear();

	//m_GoalPosition.push_back(goal);
	mPath->clearWaypoints();
	mPath->addWaypoint(goal);

	//add new marker
	if (marker) {
		addWaypointMarker(getPosition(), goal);
	}

	mNextWaypoint = 0;

	theApp->BroadcastEvent(Event::NEW_GOAL, this);

	return true;
}

osg::Vec3 Unit::getGoalPosition() {
	//the next goal position
	osg::Vec3 pos = mPath->getTarget(getPosition());

	return pos;
}

bool Unit::setGoalPath(PathInterpolation path, bool marker)
{
        // copy the path into a new instance which is allocated on the heap
        mPath = new PathInterpolation(path);

        //remove markers
        for (unsigned int i = 0; i < mWaypointMarkers.size(); i++) {
                theApp->getTerrain()->removeTerrainOverlayChild(mWaypointMarkers[i].get());
        }
        mWaypointMarkers.clear();

        //add new markers
        if (marker)
          {
            unsigned int num_of_wps = mPath->getNumOfWaypoints();

            if (num_of_wps > 0)
                addWaypointMarker(getPosition(), mPath->getWaypoint(0));
            for (unsigned int t = 1; t<num_of_wps-1; t++)
              {
                addWaypointMarker(mPath->getWaypoint(t), mPath->getWaypoint(t+1));
              }
        }
        mNextWaypoint = 0;

        theApp->BroadcastEvent(Event::NEW_GOAL, this);

        return true;
}

osg::Vec3 Unit::getFinalGoalPosition() {
	return mPath->getFinalWaypoint();
}

/**
 * Sets the position of the unit
 *
 * Should be initially called by subclasses
 */
bool Unit::setPosition(osg::Vec3 pos)
{
	try {
		pos = validatePosition(pos);
	} catch (dtUtil::Exception &e) {
		return false;
	}

	for (unsigned int i = 0; i < GetNumChildren(); i++)
	{
		dtCore::Transform current_tr;
		dtCore::Transformable* next_object = dynamic_cast<dtCore::Transformable*>(GetChild(i));
		next_object->GetTransform(current_tr);
		current_tr.SetTranslation( pos );
		next_object->SetTransform(current_tr);
	}

	return true;
}

osg::Vec3 Unit::getPosition()
{
	if (mPredictedUnit) {
		return m_StartPosition;
	}
	//return the units position based on the centre of its bounding sphere
	osg::Vec3 center;
	float radius;
	GetBoundingSphere(&center, &radius);
	return center;
}

/**
 * Clamps z-value to terrain height if necessary
 */
osg::Vec3 Unit::validatePosition(osg::Vec3 pos) {
	if(m_IsFlying && theApp->IsValidXYPosition(pos.x(), pos.y()))
	{
		if (!theApp->IsValidPosition(pos)) {
			pos = theApp->mapPointToTerrain(pos);
		}
		return pos;
	}
	else if( !m_IsFlying && theApp->IsValidXYPosition(pos.x(), pos.y()) )
	{
		// set the height just above the terrain
		double height;
		theApp->GetTerrainHeight(pos.x(), pos.y(), height);

		return osg::Vec3(pos.x(), pos.y(), height);
	}
	else
	{
		std::ostringstream oss;
		oss << "Invalid position of " << GetName() << ": " << pos << std::endl;
		LOG_WARNING(oss.str());
		throw new dtUtil::Exception(oss.str(), "Unit.cpp", __LINE__);
	}
}

bool Unit::setHeading(double heading)
{
	// store current position to reset it after rotation
	osg::Vec3 curr_pos = getPosition();

	for (unsigned int i = 0; i < GetNumChildren(); i++)
	{
		dtCore::Transform current_tr;
		dtCore::Transformable* next_object = dynamic_cast<dtCore::Transformable*>(GetChild(i));
		next_object->GetTransform(current_tr);
		current_tr.SetRotation( heading, 0, 0 );
		next_object->SetTransform(current_tr);
	}

	setPosition(curr_pos);

	return true;
}

double Unit::getHeading()
{
	if (mPredictedUnit) {
		return m_StartHeading;
	}
	// define heading by the first object's direction
	osg::Vec3 current_rot;
	dtCore::Transform current_tr;
	if (GetNumChildren() == 0) return NAN;
	dynamic_cast<dtCore::Transformable*>(GetChild(0))->GetTransform(current_tr);
	current_tr.GetRotation(current_rot);
	return current_rot.x();
}

osg::Quat Unit::getQuat() {
	dtCore::Transform transform;
	osg::Matrix mat;
	osg::Quat q;

	if (GetNumChildren() == 0) return osg::Quat(NAN,NAN,NAN,NAN);
	dynamic_cast<dtCore::Transformable*>(GetChild(0))->GetTransform(transform);
	transform.GetRotation(mat);
	mat.get(q);

	return q;
}

osg::Vec3 Unit::getHeadingVector() {
	return getQuat()*osg::Vec3(0,1,0);
}

osg::Vec3 Unit::getNormal() {
	return getQuat()*osg::Vec3(0,0,1);
}

float Unit::getHeadingToPosition(osg::Vec3 goalPos) {
	osg::Vec3 pos, pt, cv;
	osg::Vec2 p, v, v90;

	//get position of unit
	pos = getPosition();
	//calculate vector to target
	pt = goalPos - pos;
	//project to 2D plane
	p = osg::Vec2(pt[0], pt[1]);
	p.normalize();

	//get direction vector of unit
	cv = getHeadingVector();
	//project to 2D plane
	v = osg::Vec2(cv[0],cv[1]);
	v.normalize();

	//vector rotated by 90 degrees
	v90 = osg::Vec2(-v[1], v[0]);

	//use dot product formula to get angle (0-PI)
	float vp = v*p;
	if (vp < -1) vp = -1;
	if (vp > 1) vp = 1;
	float angle = acos(vp);

	//find out whether angle is to the left or right
	float beta = p * v90;

	// actually go left
	if (beta > 0)
		angle = -angle;

	return angle;
}

/**
 * Returns the angle from the z=0 plane of the unit, in the direction of the goal
 */
float Unit::getPitchToPosition(osg::Vec3 goalPos) {
	dtCore::Transform transform;
	osg::Matrix mat;
	osg::Quat q;
	osg::Vec3 pos, targetDir, unitDir;
	osg::Vec3 v90;

	//get position of unit
	pos = getPosition();
	//calculate vector to target
	targetDir = goalPos - pos;
	//project to 2D plane
	targetDir[2] = 0;
	targetDir.normalize();

	//get direction vector of unit
	unitDir = getHeadingVector();
	unitDir.normalize();
	//rotate to heading around normal
	osg::Vec3 unitNorm = getNormal();
	osg::Matrixf rotateHeading = osg::Matrixf::rotate(getHeadingToPosition(goalPos), unitNorm);
	osg::Vec3 aimHeading = rotateHeading * unitDir;

	//use dot product formula to get angle (0-PI)
	float angle = acos(fmin(1,aimHeading * targetDir));

	//find out whether angle is up or down
	float beta = targetDir * unitNorm;

	// actually go up (or down?)
	if (beta < 0)
		angle = -angle;

	return angle;
}

/**
 * Gets the angle of rotation needed to head towards the goal position
 */
float Unit::getAngleToGoal()
{
	return getHeadingToPosition(getGoalPosition());
}

float Unit::getDistanceToGoal()
{
	return (getGoalPosition() - getPosition()).length();
}

void Unit::updateShaderParams()
{
   //double simTime = GetGameActorProxy().GetGameManager()->GetSimulationTime();
   double simTime = theApp->getSimTime();

   // If we have a shader, then calculate a number from 0 to 1 over X seconds and apply it.
   if (mCurrentShader.valid())
   {
      // Texture dilation
      if (mTextureDilationParam != NULL)
      {
         float temp = simTime/mShaderTextureCycleTime;
         mTextureDilationParam->SetValue(temp - (int) temp);
         // old code to make the num go from 1 to 0 to 1 over X seconds
         //float timeDistort = fabs(variance/(float)CYCLETIME * 2.0f - 1.0f);
      }

      // X Dilation
      if (mMoveXDilationParam != NULL)
      {
         float temp = simTime/mShaderMoveXCycleTime;
         mMoveXDilationParam->SetValue(temp - (int) temp);
      }

      // Y Dilation
      if (mMoveYDilationParam != NULL)
      {
         float temp = simTime/mShaderMoveYCycleTime;
         mMoveYDilationParam->SetValue(temp - (int) temp);
      }

      // Z Dilation
      if (mMoveZDilationParam != NULL)
      {
         float temp = simTime/mShaderMoveZCycleTime;
         mMoveZDilationParam->SetValue(temp - (int) temp);
      }
   }

}

void Unit::applyShader()
{
   if (mCurrentShader != NULL && mCurrentShader->GetName() == mCurrentShaderName)
   {
      // don't reload stuff or we kill our processing.
      return;
   }

   // clean up any previous shaders, if any
   std::vector<dtCore::Object*>::iterator it;
   dtCore::ShaderManager::GetInstance().UnassignShaderFromNode( *GetOSGNode() );

   dtCore::ShaderProgram *templateShader = dtCore::ShaderManager::GetInstance().
      FindShaderPrototype(mCurrentShaderName,"TargetShaders");
   if (templateShader != NULL)
   {
        mCurrentShader = dtCore::ShaderManager::GetInstance().
         	AssignShaderFromPrototype( *templateShader, *GetOSGNode() );

        mTextureDilationParam = dynamic_cast<dtCore::ShaderParamFloat*> (mCurrentShader->FindParameter("TimeDilation"));
      	mMoveXDilationParam = dynamic_cast<dtCore::ShaderParamFloat*> (mCurrentShader->FindParameter("MoveXDilation"));
	    mMoveYDilationParam = dynamic_cast<dtCore::ShaderParamFloat*> (mCurrentShader->FindParameter("MoveYDilation"));
      	mMoveZDilationParam = dynamic_cast<dtCore::ShaderParamFloat*> (mCurrentShader->FindParameter("MoveZDilation"));

    	updateShaderParams();
   }
   else
   {
      LOG_ERROR("Unit could not load shader for group[TargetShaders] with name [" + mCurrentShaderName + "]");
      mCurrentShader = NULL;
      mTextureDilationParam = NULL;
      mMoveXDilationParam = NULL;
      mMoveYDilationParam = NULL;
      mMoveZDilationParam = NULL;
   }
}

void Unit::setSelected(bool sel) {
	unsigned int index = 0;
	if (mPath != 0) {
		index = mPath->getWaypointIndex() + 1;
	}
	mSelected = sel;
	if (sel) {
		if (getSide() == RED)
			mCurrentShaderName = "RedSelected";
		else
			mCurrentShaderName = "BlueSelected";

		if (getIsControlled() && mWaypointMarkers.size() > 0) {
			for (unsigned int i=index; i<mWaypointMarkers.size(); i++) {
				theApp->getTerrain()->addTerrainOverlayChild(mWaypointMarkers[i].get());
			}
			updateWaypointMarker();
		}
	} else {
		if (getSide() == RED)
			mCurrentShaderName = "Red";
		else
			mCurrentShaderName = "Blue";

		if (getIsControlled() && mWaypointMarkers.size() > 0) {
			for (unsigned int i=0; i<mWaypointMarkers.size(); i++) {
				theApp->getTerrain()->removeTerrainOverlayChild(mWaypointMarkers[i].get());
			}
		}
	}

	applyShader();
	updateUnitMarker();

	if (sel)
		theApp->BroadcastEvent(Event::SELECTED, this);
}

bool Unit::isSelected() {
	return (mCurrentShaderName == "RedSelected" || mCurrentShaderName == "BlueSelected");
}

float Unit::decrementHealth(float by) {
	m_Health -= by;
	//std::cout << "Health: " << m_Health << std::endl;
	theApp->displayUnitHealth(this, m_Health);
	//EventLog::GetInstance().log(EventLog::HURT, this);
	return m_Health;
}

bool Unit::isEnemy(Unit *unit) const {
	return (unit->getSide() != this->getSide());
}

void Unit::updateWaypointMarker() {
	if (getIsControlled() && isSelected() && mWaypointMarkers.size() > 0) {
		//get the current waypoint
		int index = mPath->getWaypointIndex();
		osg::Vec3 endPos = mPath->getWaypoint(index);
		osg::Vec3 startPos = getPosition();

		//direction
		osg::Vec3 dir = osg::Vec3(endPos[0], endPos[1], 0) - osg::Vec3(startPos[0], startPos[1], 0);
		//direction rotated by 90 degrees
		osg::Vec3 offset = osg::Vec3(-dir[1], dir[0], 0);
		offset.normalize();

		//get the geometry and vertices that are deep in the dtCore::Object
		osg::Geometry* lineGeom = dynamic_cast<osg::Geode*>(mWaypointMarkers[index]->GetMatrixNode()->getChild(1))->getDrawable(0)->asGeometry();
		osg::Vec3Array* vertices = dynamic_cast<osg::Vec3Array*>(lineGeom->getVertexArray());
		//modify the vertices
		vertices->at(0) = offset;
	    vertices->at(1) = -offset;
	    vertices->at(2) = dir-offset;
	    vertices->at(3) = dir+offset;
	    //trigger redraw
	    lineGeom->setVertexArray(vertices);

		//position
	    dtCore::Transform t;
    	t.SetTranslation(startPos);
    	mWaypointMarkers.at(index)->SetTransform(t);
	}
}

dtCore::Object* Unit::createWaypointMarker(osg::Vec3 startPos, osg::Vec3 endPos) {
	dtCore::Object* waypointMarker = new dtCore::Object("WaypointMarker");

	osg::MatrixTransform *xform = waypointMarker->GetMatrixNode();

	dtCore::RefPtr<osg::Geode> markerGeode = new osg::Geode();
	markerGeode->setName("MarkerGeode");

	dtCore::RefPtr<osg::Geometry> lineGeom = new osg::Geometry();

	//direction
	osg::Vec3 dir = osg::Vec3(endPos[0], endPos[1], 0) - osg::Vec3(startPos[0], startPos[1], 0);
	//direction rotated by 90 degrees
	osg::Vec3 offset = osg::Vec3(-dir[1], dir[0], 0);
	offset.normalize();

	dtCore::RefPtr<osg::Vec3Array> vertices = new osg::Vec3Array;
    vertices->push_back(offset);
    vertices->push_back(-offset);
    vertices->push_back(dir-offset);
    vertices->push_back(dir+offset);

    lineGeom->setVertexArray(vertices.get());

    lineGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POLYGON,0,4));

    markerGeode->addDrawable(lineGeom.get());

    osg::Material *mat = new osg::Material();
    mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(0.0f,0.8f,0.2f, 0.5f));
    mat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(0.0f,0.8f,0.2f, 1.f));
    mat->setEmission(osg::Material::FRONT_AND_BACK,osg::Vec4(0.0f,0.8f,0.2f, 1.f));

    osg::StateSet *ss = markerGeode->getOrCreateStateSet();
    ss->setAttributeAndModes(mat, osg::StateAttribute::OVERRIDE |
         osg::StateAttribute::PROTECTED | osg::StateAttribute::ON);
    ss->setMode(GL_BLEND,osg::StateAttribute::OVERRIDE |
         osg::StateAttribute::PROTECTED | osg::StateAttribute::ON);
    ss->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);

    xform->addChild(markerGeode.get());

	dtCore::Transform t;
	t.SetTranslation(startPos);
	waypointMarker->SetTransform(t);

	return waypointMarker;
}

void Unit::addWaypointMarker(osg::Vec3 startPos, osg::Vec3 endPos) {
	if (getIsControlled() && !theApp->isHeadless()) {
		dtCore::RefPtr<dtCore::Object> waypointMarker = createWaypointMarker(startPos, endPos);

		mWaypointMarkers.push_back(waypointMarker);

		if (isSelected()) {
			theApp->getTerrain()->addTerrainOverlayChild(waypointMarker.get());
		}
	}
}

void Unit::addUnitMarker() {
	if (!theApp->isHeadless()) {
		//Create unit marker

		mUnitMarker = new dtCore::Object("UnitMarker");

		osg::MatrixTransform *xform = mUnitMarker->GetMatrixNode();

		mUnitMarkerGeode = new osg::Geode();
		mUnitMarkerGeode->setName("UnitMarkerGeode");
		osg::TessellationHints* hints = new osg::TessellationHints;
		hints->setDetailRatio(0.5f);

		//cones for soldiers, spheres for tanks
		if (GetName().find("Soldier") != std::string::npos) {
		    dReal radius = 2;
		    dReal length = 7;
		    mUnitMarkerGeode->addDrawable(
		    	new osg::ShapeDrawable(new osg::Cone(osg::Vec3(0, 0, 0), radius, length), hints ) );
		} else {
			dReal radius = 7;
		    mUnitMarkerGeode->addDrawable(
		    	new osg::ShapeDrawable(new osg::Sphere(osg::Vec3(0, 0, 0), radius), hints ) );
		}

	    osg::PolygonOffset* polyoffset = new osg::PolygonOffset;
	    polyoffset->setFactor(-1.0f);
	    polyoffset->setUnits(-1.0f);

	    osg::StateSet *ss = mUnitMarkerGeode->getOrCreateStateSet();
	    ss->setMode(GL_BLEND,osg::StateAttribute::OVERRIDE |
	         osg::StateAttribute::PROTECTED | osg::StateAttribute::ON);
	    ss->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
	    ss->setAttributeAndModes(polyoffset,osg::StateAttribute::OVERRIDE |
	         osg::StateAttribute::PROTECTED | osg::StateAttribute::ON);

	    xform->addChild(mUnitMarkerGeode.get());

		dtCore::Transform t;
		t.SetTranslation(getPosition());
		mUnitMarker->SetTransform(t);

		//set material attributes
		setUnitMarkerAlpha(0.5);

		theApp->getTerrain()->addTerrainOverlayChild(mUnitMarker.get());
	}
}

void Unit::setUnitMarkerAlpha(float alpha) {
	mUnitMarkerAlpha = alpha;
	updateUnitMarker();
}

void Unit::updateUnitMarker() {
	osg::Material *mat;
	//set unit marker material attributes
	if (mUnitMarkerGeode != NULL) {
		osg::StateSet *ss = mUnitMarkerGeode->getOrCreateStateSet();
		mat = dynamic_cast<osg::Material*>(ss->getAttribute(osg::StateAttribute::MATERIAL));
		if (mat == NULL) {
			//create and assign material, if not already created
			mat = new osg::Material();
			ss->setAttributeAndModes(mat, osg::StateAttribute::OVERRIDE |
		    	osg::StateAttribute::PROTECTED | osg::StateAttribute::ON);
		}
		//set material colour & alpha depending on side and active selection (selected units are brighter, alpha depends on zoom level)
		if (!mSelected) {
		    if (getSide() == RED) {
		    	mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(0.4f,0.0f,0.0f, mUnitMarkerAlpha));
		    	mat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(0.2f,0.0f,0.0f, 1.f));
		    	mat->setEmission(osg::Material::FRONT_AND_BACK,osg::Vec4(0.1f,0.0f,0.0f, 1.f));
		    } else {
		    	mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(0.0f,0.0f,0.4f, mUnitMarkerAlpha));
		    	mat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(0.0f,0.0f,0.2f, 1.f));
		    	mat->setEmission(osg::Material::FRONT_AND_BACK,osg::Vec4(0.0f,0.0f,0.1f, 1.f));
		    }
		} else {
			if (getSide() == RED) {
		    	mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(1.f,0.0f,0.0f, mUnitMarkerAlpha));
		    	mat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(1.f,0.0f,0.0f, 1.f));
		    	mat->setEmission(osg::Material::FRONT_AND_BACK,osg::Vec4(1.f,0.0f,0.0f, 1.f));
		    } else {
		    	mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(0.0f,0.0f,1.0f, mUnitMarkerAlpha));
		    	mat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(0.0f,0.0f,1.0f, 1.f));
		    	mat->setEmission(osg::Material::FRONT_AND_BACK,osg::Vec4(0.0f,0.0f,1.0f, 1.f));
		    }
		}
	}
}

osg::Vec3 Unit::getVelocity() {
	osg::Vec3 vel = osg::Vec3(0,0,0);

	osg::Vec3 dist = getPosition() - mLastPosition;
	double time = theApp->getSimTime() - mLastPositionTime;

	if (time != 0) {
		vel = dist/time;
	}
	return vel;
}

osg::Vec3 Unit::getObservedPosition() {
	return mObservedPosition;
}

bool Unit::isObserved() {
	return (mObservedPositionTime != 0);
}

/*
 * If we have a better estimate of position then update the observed position and variance
 */
osg::Vec3 Unit::setObservedPosition(osg::Vec3 centre, float radius, float resolution) {
	float d = radius / resolution;
	float viewVariance = pow(d/2,2);

	if (viewVariance <= getObservedPositionVariance()) {
		osg::Vec3 pos = getPosition();
		//quantise position based on resolution given
		float x0 = centre.x()-radius;
		float y0 = centre.y()-radius;
		float x = d * floor((pos.x() - x0)/d) + x0 + d/2;
		float y = d * floor((pos.y() - y0)/d) + y0 + d/2;
		osg::Vec3 quantised(x,y,0);
		//save
		mObservedPosition = theApp->mapPointToTerrain(quantised);
		mObservedPositionVariance = viewVariance;
		mObservedPositionTime = theApp->getSimTime();
	}
	return mObservedPosition;
}

double Unit::getObservedPositionVariance() {
	//distance increases linearly with maxSpeed, variance is square of distance
	double timeDiff = theApp->getSimTime()-mObservedPositionTime;
	double dist = timeDiff * getMaxSpeed();
	//mObservedPositionVariance is the variance reported by the observer
	double variance = mObservedPositionVariance + (dist * dist);

	//set variance to the max when we have no observations
	if (mObservedPositionTime == 0) {
		variance = mObMaxVariance;
	}

	return variance;
}

/** Unused. */
bool Unit::isVisible() {
	return mVisible;
}

void Unit::setVisible(bool visibility = true) {
	mVisible = visibility;
}

osg::BoundingBox Unit::getObservedBoundingBox() {
	//get radius of observation
	float distance = sqrt(getObservedPositionVariance());
	osg::Vec3 d = osg::Vec3(distance,distance,2);

	osg::Vec3 pos = getObservedPosition();
	osg::Vec3 min = pos - d;
	osg::Vec3 max = pos + d;

	return osg::BoundingBox(min, max);
}

osg::BoundingBox Unit::getMaxBoundingBox() {
	//get radius of observation
	float distance = mObMaxDistance;
	osg::Vec3 d = osg::Vec3(distance,distance,2);

	osg::Vec3 pos = getPosition();
	osg::Vec3 min = pos - d;
	osg::Vec3 max = pos + d;

	return osg::BoundingBox(min, max);
}


/*UnitGroup* Unit::getGroup() {

}

void Unit::setGroup(UnitGroup* group) {

}*/
