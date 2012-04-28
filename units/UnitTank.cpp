#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wextra"

#include <dtCore/transform.h>
#include <dtUtil/boundingshapeutils.h>

#pragma GCC diagnostic warning "-Wextra"

#include "UnitTank.h"
#include "../HTapp.h"
#include "../eventlog.h"
//#include "findNodeVisitor.h"
//#include <osgSim/DOFTransform>

UnitTank::UnitTank(const std::string& name, osg::Vec3 position,
			double facing, UnitSide side, bool controlled, int client, int id)
		: UnitFiring(name, position, facing, side, controlled, false, false, client, id)
{
	//default variables
	mWheelX = 1.6f;
	mWheelYFront = 2.0f;
	mWheelYRear = 1.8f;
	mWheelZ = 0.3f;
	mWheelScale = 0.5f;
	mVehicleMass = 100;
	mWheelMass = 10;
	mSuspensionERP = 0.2;
	mSuspensionCFM = 0.001;
	mTurnERP = 0.2;
	mTurnCFM = 0.001;
	mThresholdTurret = 0.05;
	mThresholdBarrel = 0.01;
	mMaxSpeed = 60;
	mAverageSpeed = 30;
	mSlowSpeed = 15;
	mSlowingDist = 20;
	mStoppingDist = 5;
	mTurningRate = 4;
	mMaxForce = 500;

	//if available, override defaults from config file
	parseUnitConfigFile("unitTank.xml");

	mPath = new PathInterpolation(20, 1);
}

UnitTank::~UnitTank() {
	//remove joints
	if (!mPredictedUnit) {
		for(int i = 0; i < 4; i++)
		{
			dJointDestroy(mJoints[i]);
		}
//		dJointDestroy(mJointTurret);
//		dJointDestroy(mJointBarrel);
	}
}

UnitTank* UnitTank::clone() {
	return new UnitTank(*this);
}

bool UnitTank::applyConfigData(const UnitConfigData& data) {
	return (applyUnitConfigData(data) && applyUnitFiringConfigData(data) && applyUnitTankConfigData(data));
}

bool UnitTank::applyUnitTankConfigData(const UnitConfigData& data) {
	mMaxSpeed = data.movement.max_speed;
	mAverageSpeed = data.movement.average_speed;
	mSlowSpeed = data.movement.slow_speed;
	mSlowingDist = data.movement.slowing_dist;
	mStoppingDist = data.movement.stopping_dist;
	mTurningRate = data.movement.turning_rate;

	mMaxForce = data.physics.max_force;

	mWheelX = data.wheels.x;
	mWheelYFront = data.wheels.y_front;
	mWheelYRear = data.wheels.y_rear;
	mWheelZ = data.wheels.z;
	mWheelScale = 0.5f;
	mWheelMass = data.wheels.mass;

	mVehicleMass = data.physics.mass;
	mSuspensionERP = data.physics.suspension_erp;
	mSuspensionCFM = data.physics.suspension_cfm;
	mTurnERP = data.physics.turn_erp;
	mTurnCFM = data.physics.turn_cfm;

	mThresholdTurret = data.firing.turret;
	mThresholdBarrel = data.firing.barrel;

	return true;
}

void UnitTank::LoadMeshes()
{
	const std::string hull_model = "StaticMeshes/T72hull.ive";
	const std::string turret_model = "StaticMeshes/T72turret.ive";
	const std::string barrel_model = "StaticMeshes/T72gun.ive";
	const std::string wheel_model = "StaticMeshes/wheel.flt";

	//load vehicle body model
	mHull = new dtCore::Object("Hull");

    if (!mHull->LoadFile(hull_model))
    {
    	LOG_ERROR("Cannot find tank hull model.");
    	return;
    }

	//load wheel models
	for(int i = 0; i < 4; i++)
	{
		mWheels[i] = new dtCore::Object("Wheel");
		if (!mWheels[i]->LoadFile(wheel_model))
		{
    		LOG_ERROR("Cannot find wheel model.");
    		return;
	    }
	}

	/*mTurret = new dtCore::Object("Turret");
	if (!mTurret->LoadFile(turret_model))
	{
		LOG_ERROR("Cannot find turret model.");
		return;
    }

	mBarrel = new dtCore::Object("Barrel");
	if (!mBarrel->LoadFile(barrel_model))
	{
		LOG_ERROR("Cannot find barrel model.");
		return;
    }*/

	// move objects into relative position

	// hull, turret and barrel does not need moving
	// their mesh is defined in a proper relative position

	// wheels
	dtCore::Transform trans1;
	osg::Vec3 hpr = osg::Vec3(0,0,0);
	osg::Vec3 sx = osg::Vec3(mWheelScale, mWheelScale, mWheelScale);

	trans1.Set(osg::Vec3(-mWheelX, mWheelYFront, mWheelZ), hpr);
	mWheels[0]->SetTransform(trans1);
	mWheels[0]->SetScale(sx);
	trans1.Set(osg::Vec3(mWheelX, mWheelYFront, mWheelZ), hpr);
	mWheels[1]->SetTransform(trans1);
	mWheels[1]->SetScale(sx);
	trans1.Set(osg::Vec3(-mWheelX, -mWheelYRear, mWheelZ), hpr);
	mWheels[2]->SetTransform(trans1);
	mWheels[2]->SetScale(sx);
	trans1.Set(osg::Vec3(mWheelX, -mWheelYRear, mWheelZ), hpr);
	mWheels[3]->SetTransform(trans1);
	mWheels[3]->SetScale(sx);

	// add objects to member list
	AddChild(mHull.get());
    //AddChild(mTurret.get());
    //AddChild(mBarrel.get());
	for (int i = 0; i<4; i++) AddChild( mWheels[i].get() );

	// register networking objects (do not include wheels)
	RegisterNetworkingChild(mHull.get(), hull_model);
	//RegisterNetworkingChild(mTurret.get(), turret_model);
	//RegisterNetworkingChild(mBarrel.get(), barrel_model);
}

void UnitTank::initPhysics()
  {
	dMass mass;
	//SetCollisionBox();
	mHull->SetCollisionDetection(true);
	dMassSetBoxTotal(&mass, mVehicleMass, 2.0f, 1.06f, 4.0f); //x, y, z
	mHull->SetMass(&mass);
	dGeomSetCategoryBits(mHull->GetGeomID(), 0x00000001); //jeep vehicle body, not wheels
	dGeomSetCollideBits(mHull->GetGeomID(), 0xFFFFFFF0); //don't collide with my wheels or mis
	mHull->EnableDynamics();

	for(int i = 0; i < 4; i++)
    {
         //create wheel
         mWheels[i]->SetCollisionSphere();
         // mWheels[i]->SetCollisionCappedCylinder();
		 // mWheels[i]->SetCollisionBox();
         dMassSetCylinderTotal(&mass, mWheelMass, 1, 0.45f, 0.3f);
         mWheels[i]->SetMass(&mass);
         dGeomSetCategoryBits(mWheels[i]->GetGeomID(), 0x00000002); //jeep wheel, not vehicle body
         dGeomSetCollideBits(mWheels[i]->GetGeomID(), 0xFFFFFFF0); //don't collide with my vehicle body
         // mWheels[i]->RenderCollisionGeometry();
         mWheels[i]->EnableDynamics();
         dBodySetFiniteRotationMode(mWheels[i]->GetBodyID(), 1);

         //create joint
         mJoints[i] = dJointCreateHinge2(theApp->GetScene()->GetWorldID(), 0);
         dJointAttach(mJoints[i], mHull->GetBodyID(), mWheels[i]->GetBodyID());
    }

    dJointSetHinge2Anchor(mJoints[0], -mWheelX, mWheelYFront, mWheelZ);
    dJointSetHinge2Anchor(mJoints[1], mWheelX, mWheelYFront, mWheelZ);
    dJointSetHinge2Anchor(mJoints[2], -mWheelX, -mWheelYRear, mWheelZ);
    dJointSetHinge2Anchor(mJoints[3], mWheelX, -mWheelYRear, mWheelZ);

    for(int i = 0; i < 4; i++)
    {
		dJointSetHinge2Axis1(mJoints[i], 0,0,1); //up
		dJointSetHinge2Axis2(mJoints[i], 1,0,0); //in
		dJointSetHinge2Param(mJoints[i], dParamSuspensionERP, mSuspensionERP);
		dJointSetHinge2Param(mJoints[i], dParamSuspensionCFM, mSuspensionCFM);
		dJointSetHinge2Param(mJoints[i], dParamLoStop, 0);
		dJointSetHinge2Param(mJoints[i], dParamHiStop, 0);
		dJointSetHinge2Param(mJoints[i], dParamStopERP, mTurnERP);
		dJointSetHinge2Param(mJoints[i], dParamStopCFM, mTurnCFM);
	}
/*
	// mTurret->SetCollisionCappedCylinder(1,0.5);
	// mTurret->SetCollisionCappedCylinder();
	// mTurret->RenderCollisionGeometry();
    mTurret->SetCollisionDetection(true);
	mTurret->EnableDynamics();
	dMassSetCylinderTotal(&mass, 20, 3, 1.0f, 2.0f);
	mTurret->SetMass(&mass);
	dGeomSetCategoryBits(mTurret->GetGeomID(), 0x00000003);
	dGeomSetCollideBits(mTurret->GetGeomID(), 0xFFFFFFF0);

	// mBarrel->SetCollisionCappedCylinder(0.1,4);
	// mBarrel->SetCollisionCappedCylinder();
	// mBarrel->RenderCollisionGeometry();
	mBarrel->SetCollisionDetection(true);
	mBarrel->EnableDynamics();
	dMassSetCylinderTotal(&mass, 10, 2, 2.0f, 2.0f);
	mBarrel->SetMass(&mass);
	dGeomSetCategoryBits(mBarrel->GetGeomID(), 0x00000004);
	dGeomSetCollideBits(mBarrel->GetGeomID(), 0xFFFFFFF0);

	//turret joint
    mJointTurret = dJointCreateHinge(theApp->GetScene()->GetWorldID(), 0);
    dJointAttach(mJointTurret, mHull->GetBodyID(), mTurret->GetBodyID());
    dJointSetHingeAnchor(mJointTurret, 0, 0, 2);

    dJointSetHingeAxis(mJointTurret, 0,0,1); //up
	dJointSetHingeParam(mJointTurret, dParamStopERP, mTurnERP);
	dJointSetHingeParam(mJointTurret, dParamStopCFM, mTurnCFM);
	dJointSetHingeParam(mJointTurret, dParamVel,0); //+ve = clockwise
	dJointSetHingeParam(mJointTurret, dParamFMax,mMaxForce);

    //barrel joint
    mJointBarrel = dJointCreateHinge(theApp->GetScene()->GetWorldID(), 0);
    dJointAttach(mJointBarrel, mTurret->GetBodyID(), mBarrel->GetBodyID());
    dJointSetHingeAnchor(mJointBarrel, 0, 1, 2);

    dJointSetHingeAxis(mJointBarrel, 1,0,0); //tilt
	//dJointSetHingeParam(mJointBarrel, dParamLoStop, -1);
	//dJointSetHingeParam(mJointBarrel, dParamHiStop, 0.25);
	dJointSetHingeParam(mJointBarrel, dParamStopERP, mTurnERP);
	dJointSetHingeParam(mJointBarrel, dParamStopCFM, mTurnCFM);
	dJointSetHingeParam(mJointBarrel, dParamVel,0); //negative = up
	dJointSetHingeParam(mJointBarrel, dParamFMax,mMaxForce); //500 = good
*/
}

/**
 * Update the tank to steer towards the target
 * Called by PreFrame
 */
void UnitTank::updatePhysics() {
	//update speed based on current terrain type (road=fast, city/none=medium, forest=slow)
	osg::Vec3 pos = getPosition();

	float speed = getSpeed(pos[0], pos[1]);

	// parameters
	/*static const float MAX_FORCE = 500;
	static const float SLOWING_DIST = 20;
	static const float STOPPING_DIST = 5;
	static const float MAX_SPEED = 30;
	static const float TURNING_RATE = 4;*/

	float angle = getAngleToGoal();
	float dist = getDistanceToGoal();

	//modify speed to left and right wheels in order to steer
	float speeddiff = angle/osg::PI;

	//cap
	if (speeddiff > 1) speeddiff = 1;
	if (speeddiff < -1) speeddiff = -1;

	float gas = mMaxForce;
	float speed_factor = 1;

	//when close, slow down
	if (dist < mSlowingDist) {
		if (dist < mStoppingDist) speed_factor = 0;
		else speed_factor = (dist-mStoppingDist)/(mSlowingDist-mStoppingDist);
	}

	//speed-dependent turning rate
	speeddiff *= mTurningRate / (1 + getVelocity().length());

	//assign to right and left
	float leftspeed = speed_factor*speed*(1 + speeddiff);
	float rightspeed = speed_factor*speed*(1 - speeddiff);

	//set speed and gas on joints
	for (int i=0; i<4; i++) {
		// motor
		dJointSetHinge2Param (mJoints[i],dParamVel2,((i % 2)==0) ? leftspeed : rightspeed);
		dJointSetHinge2Param (mJoints[i],dParamFMax2,gas);
	}
/*
	std::ostringstream oss;
	oss << "Goal: " << getGoalPosition() << " Angle: " << angle << " Distance: " << dist << std::endl;
	oss << "Target speed: " << speed << " Target angle: " << speeddiff << std::endl;
	oss << "Left speed: " << leftspeed << " Right speed: " << rightspeed;
	theApp->hudGUI->st->setText(oss.str());
*/
}

osg::Vec3 UnitTank::validatePosition(osg::Vec3 pos) {

	pos = Unit::validatePosition(pos);

	// increase height with the bounding box
	osg::BoundingBox UnitBB;
	GetBoundingBox(UnitBB, *this);
	pos[2] += (UnitBB.zMax()-UnitBB.zMin())/4;

	return pos;
}

/**
 * Move turret and barrel into position
 */
void UnitTank::aim() {
	//calc heading and pitch
/*	calcAimParams();

	dReal v;
	//move turret
	v = mAimHeading - dJointGetHingeAngle (mJointTurret);
	if (v > osg::PI || v < -osg::PI) v = -v;
	if (v > 0.1) v = 0.1;
	if (v < -0.1) v = -0.1;
	v *= 10.0;
	dJointSetHingeParam(mJointTurret,dParamVel,v);
	dJointSetHingeParam(mJointTurret,dParamFMax,500.0);

	//move barrel
	v = (-mAimPitch) - dJointGetHingeAngle (mJointBarrel);
	if (v > 0.1) v = 0.1;
	if (v < -0.1) v = -0.1;
	v *= 10.0;
	dJointSetHingeParam(mJointBarrel,dParamVel,v);
	dJointSetHingeParam(mJointBarrel,dParamFMax,500.0);
*/
	/*std::ostringstream oss;
	oss << mAimPitch << "," << dJointGetHingeAngle (mJointBarrel) << std::endl;
	theApp->hudGUI->st->setText(oss.str());*/
}

/**
 * If turret and barrel are aimed (to within a certain threshold)
 */
bool UnitTank::isAimed() {
/*	return ((fabs(dJointGetHingeAngle(mJointTurret)-mAimHeading) <= mThresholdTurret)
		&& (fabs(dJointGetHingeAngle(mJointBarrel)+mAimPitch) <= mThresholdBarrel));
*/ return true;
}

/**
 * Stop the turret from moving
 */
void UnitTank::freezeAim() {
/*	//turret
	dJointSetHingeParam(mJointTurret,dParamVel,0);
	dJointSetHingeParam(mJointTurret,dParamFMax,500.0);
	//barrel
	dJointSetHingeParam(mJointBarrel,dParamVel,0);
	dJointSetHingeParam(mJointBarrel,dParamFMax,500.0);
*/
}

void UnitTank::resetAim() {
/*	dReal v;
	//move turret
	v = -dJointGetHingeAngle (mJointTurret);
	if (v > osg::PI || v < -osg::PI) v = -v;
	if (v > 0.1) v = 0.1;
	if (v < -0.1) v = -0.1;
	v *= 10.0;
	dJointSetHingeParam(mJointTurret,dParamVel,v);
	dJointSetHingeParam(mJointTurret,dParamFMax,500.0);

	//move barrel
	v = -dJointGetHingeAngle (mJointBarrel);
	if (v > 0.1) v = 0.1;
	if (v < -0.1) v = -0.1;
	v *= 10.0;
	dJointSetHingeParam(mJointBarrel,dParamVel,v);
	dJointSetHingeParam(mJointBarrel,dParamFMax,500.0);

	if (((fabs(dJointGetHingeAngle(mJointTurret)-mAimHeading)) <= mThresholdTurret)
		&& ((fabs(dJointGetHingeAngle(mJointBarrel)+mAimPitch)) <= mThresholdBarrel)) {
			freezeAim();
			mAimReset = false;
	}*/
}

/**
 * launch projectile based on turret's actual position (projectile launches from the centre of the turret)
 * (annoyingly the barrel angle is opposite)
 */
bool UnitTank::fire() {
	calcAimParams();
//	return launch(getProjectileLaunchPos(), calcFireDirection(dJointGetHingeAngle(mJointTurret), -dJointGetHingeAngle(mJointBarrel)));
	return launch(getProjectileLaunchPos(), calcFireDirection(mAimHeading, mAimPitch));
}

osg::Vec3 UnitTank::getProjectileLaunchPos() {
	dtUtil::BoundingBoxVisitor bbv;
    GetChild(1)->GetOSGNode()->accept(bbv);
    return bbv.mBoundingBox.center();
}

float UnitTank::getMaxSpeed() {
	return mMaxSpeed;
}

float UnitTank::getSpeed(float x, float y) {
	//update speed based on current terrain type (road=fast, city/none=medium, forest=slow)
	unsigned long int layerMask = theApp->getTerrain()->GetLayerBitsAt(x, y);

	float speed = mAverageSpeed;

	if (layerMask & mRoadMask) {
		speed = mMaxSpeed;
	} else if (layerMask & mForestMask) {
		speed = mSlowSpeed;
	}

	return speed;
}
