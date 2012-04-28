#include <dtCore/transform.h>

#include "UnitSoldier.h"
#include "../HTapp.h"
#include <osg/ShapeDrawable>
#include <osg/Material>
#include <osg/PolygonOffset>
#include <osg/Geode>
#include <osg/MatrixTransform>
#include <dtAnim/sequencemixer.h>
#include <dtAnim/animnodebuilder.h>
#include <dtAnim/cal3ddatabase.h>
#include <dtAnim/cal3dmodelwrapper.h>
#include <dtUtil/functor.h>

UnitSoldier::UnitSoldier(const std::string& name, osg::Vec3 position,
			double facing, UnitSide side, bool controlled, int client, int id)
		: UnitFiring(name, position, facing, side, controlled, false, false, client, id)
{
	mSpeed = 0;
	mTurnRate = 0;
	mPrevHeight = 0;

	m_ReloadTime = 0.1;
	mProjectileMass = 0.1;
	mProjectileRadius = 0.1;
	mProjectileVelocity = 500;
	mProjectileYield = 1;
	mProjectileRange2 = 10000;
	mProjectileType = Projectile::BULLET;

	mMaxSpeed = 8;
	mAverageSpeed = 4;			//metres per second
	mSlowSpeed = 2;
	mSlowingDist = 3;
	mStoppingDist = 1.5;
	mTurningRate = 100;		//degrees per second

	mThresholdTurret = 0.05;

	//if available, override defaults from config file
	parseUnitConfigFile("unitSoldier.xml");

	mPath = new PathInterpolation(3, 1);
}

UnitSoldier::~UnitSoldier() {
	//remove joints
}

UnitSoldier* UnitSoldier::clone() {
	return new UnitSoldier(*this);
}

bool UnitSoldier::applyConfigData(const UnitConfigData& data) {
	return (applyUnitConfigData(data) && applyUnitFiringConfigData(data) && applyUnitSoldierConfigData(data));
}

bool UnitSoldier::applyUnitSoldierConfigData(const UnitConfigData& data) {
	mMaxSpeed = data.movement.max_speed;
	mSlowSpeed = data.movement.slow_speed;
	mAverageSpeed = data.movement.average_speed;
	mSlowingDist = data.movement.slowing_dist;
	mStoppingDist = data.movement.stopping_dist;
	mTurningRate = data.movement.turning_rate;

	mThresholdTurret = data.firing.turret;
	return true;
}

/**
 * Overridden from Unit because the default bounding sphere position doesn't work
 */
osg::Vec3 UnitSoldier::getPosition() {
	if (mPredictedUnit) {
		return m_StartPosition;
	}
	osg::Vec3 pos;
	dtCore::Transform t;

	mMarker->GetTransform(t);
	t.GetTranslation(pos);

	return pos;
}

/**
 * For some reason the direction vector must be rotated the opposite way...
 */
osg::Vec3 UnitSoldier::getHeadingVector() {
	return Unit::getQuat() * osg::Vec3(0,-1,0);
}
/**
 * Soldier always stands vertically
 */
osg::Vec3 UnitSoldier::getNormal() {
	return osg::Vec3(0,0,1);
}

/**
 * Load the 3D object
 */
void UnitSoldier::LoadMeshes() {
	//Make dummy geometry to perform hit tests upon
	mMarker = new dtCore::Object("Soldier");

	osg::MatrixTransform *xform = mMarker->GetMatrixNode();
	dtCore::RefPtr<osg::Geode> mUnitMarkerGeode = new osg::Geode();
	mUnitMarkerGeode->setName("SoldierMarkerGeode");

	//make a box the same approx size as the soldier
	mUnitMarkerGeode->addDrawable(
	    	new osg::ShapeDrawable(new osg::Box(osg::Vec3(0, 0, 0), 0.7, 0.5, 3.5) ) );

    osg::StateSet *ss = mUnitMarkerGeode->getOrCreateStateSet();
    ss->setMode(GL_BLEND,osg::StateAttribute::OVERRIDE |
         osg::StateAttribute::PROTECTED | osg::StateAttribute::ON);
    ss->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
    osg::Material *mat = new osg::Material();
	ss->setAttributeAndModes(mat, osg::StateAttribute::OVERRIDE |
    	osg::StateAttribute::PROTECTED | osg::StateAttribute::ON);
	mat->setDiffuse(osg::Material::FRONT_AND_BACK, osg::Vec4(0.4f,0.0f,0.0f, 0.0f));
	mat->setAmbient(osg::Material::FRONT_AND_BACK, osg::Vec4(0.2f,0.0f,0.0f, 1.f));
	mat->setEmission(osg::Material::FRONT_AND_BACK,osg::Vec4(0.1f,0.0f,0.0f, 1.f));

    xform->addChild(mUnitMarkerGeode.get());
    AddChild(mMarker.get());

    //Worlds most obscure lines of code to turn off hardware skinning of characters
    dtAnim::Cal3DDatabase& database = dtAnim::Cal3DDatabase::GetInstance();
    database.GetNodeBuilder().SetCreate(dtUtil::Functor<dtCore::RefPtr<osg::Node>, TYPELIST_1(dtAnim::Cal3DModelWrapper*)>(&database.GetNodeBuilder(),&dtAnim::AnimNodeBuilder::CreateSoftware));

    //The soldier animation
    mSoldier = new dtCore::Object("SoldierAnim");
	mAnimHelper = new dtAnim::AnimationHelper();
	mAnimHelper->LoadModel("SkeletalMeshes/marine.xml");
	mSoldier->GetMatrixNode()->addChild(mAnimHelper->GetNode());
	AddChild(mSoldier.get());

	RegisterNetworkingChild(mMarker.get(), "StaticMeshes/enemysoldier.ive");
}

/**
 * Initialise any ODE physics
 */
void UnitSoldier::initPhysics() {
	osg::Vec3 pos = getPosition();
	mPrevHeight = pos.z();
}

void UnitSoldier::applyShader() {
	//don't apply a shader to the soldiers because it makes the marker visible
}

/**
 * Rotate the soldier to face the heading
 * Rate of turn defined by mTurningRate
 */
void UnitSoldier::turnTo(double angle) {
	double turn_factor = 0;

	if (angle < 0) {
		turn_factor = 1;
	} else if (angle > 0) {
		turn_factor = -1;
	}
	//set rotation
	double heading = getHeading();
	heading += turn_factor * mTurningRate * theApp->getFrameTime();
	if (!isnan(heading)) setHeading(heading);
}

/**
 * Update the soldiers position
 * Called by Unit::update
 * (not actually physics-based movement ;-)
 */
void UnitSoldier::updatePhysics() {
	//update speed based on current terrain type (road=fast, city/none=medium, forest=slow)
	osg::Vec3 pos = getPosition();
	unsigned long int layerMask = theApp->getTerrain()->GetLayerBitsAt(pos[0], pos[1]);

	//speed based on distance from target
	float dist = getDistanceToGoal();
	float speed = 0;
	if (dist < mStoppingDist) {
		if(mAnimHelper->GetSequenceMixer().GetActiveAnimation("Idle") == NULL) {
			mAnimHelper->ClearAll(0.5f);
			mAnimHelper->PlayAnimation("Idle");
		}
		speed = 0.0f;
	} else if (dist < mSlowingDist || (layerMask & mForestMask)) {
		if(mAnimHelper->GetSequenceMixer().GetActiveAnimation("Walk") == NULL) {
			mAnimHelper->ClearAll(0.5f);
			mAnimHelper->PlayAnimation("Walk");
		}
		speed = mSlowSpeed;
	} else {
		if(mAnimHelper->GetSequenceMixer().GetActiveAnimation("Run") == NULL) {
			mAnimHelper->ClearAll(0.5f);
			mAnimHelper->PlayAnimation("Run");
		}
		if (layerMask & mRoadMask) {
			speed = mMaxSpeed;
		} else {
			speed = mAverageSpeed;
		}
	}

	//calc rotation factor
	if (dist > mStoppingDist) {
		double angle = getAngleToGoal();
		turnTo(angle);
	}

	//update the character position
	setPosition(getPosition() + (getHeadingVector() * (speed * theApp->getFrameTime())));
	//update the animation
	mAnimHelper->Update((float)theApp->getFrameTime());
}

/**
 * Rotate soldier into position
 */
void UnitSoldier::aim() {
	//only stationary soldiers can shoot
	if (getDistanceToGoal() < mStoppingDist) {
		//calc heading and pitch
		calcAimParams();
		//turn to aim
		turnTo(mAimHeading);
		//calc heading and pitch again if we're about to fire
		if (isAimed()) {
			calcAimParams();
		}
	}
}

/**
 * If soldier is standing and facing correct direction (to within a certain threshold)
 */
bool UnitSoldier::isAimed() {
	return (getDistanceToGoal() < mStoppingDist) && (fabs(mAimHeading) <= mThresholdTurret);
}

/**
 * launch projectile based on soldiers rotation
 */
bool UnitSoldier::fire() {
	osg::Vec3 unitNorm = getNormal();
	osg::Vec3 heading = getHeadingVector();
	double sigma = 0.02;

	//add some noise to the heading
	float aimHeading = mAimHeading + theApp->getRandomGaussian(sigma);

	//add some noise to the heading
	osg::Matrixf aimHeadingRotation = osg::Matrixf::rotate(aimHeading, unitNorm);
	osg::Vec3 aimHeadingVector = aimHeadingRotation * heading;

	//rotate heading by the aim pitch (+ some noise) about normal X heading
	float aimPitch = mAimPitch + theApp->getRandomGaussian(sigma);
	osg::Matrixf rotatePitch = osg::Matrixf::rotate(aimPitch, unitNorm ^ heading);
	osg::Vec3 aim = rotatePitch * aimHeadingVector;

	return launch(getProjectileLaunchPos(), aim);
}

osg::Vec3 UnitSoldier::getProjectileLaunchPos() {
	return getPosition() + osg::Vec3(0,0,1);
}

float UnitSoldier::getMaxSpeed() {
	return mMaxSpeed;
}

float UnitSoldier::getSpeed(float x, float y) {
	//update speed based on current terrain type (road=fast, city/none=medium, forest=slow)
	unsigned long int layerMask = theApp->getTerrain()->GetLayerBitsAt(x, y);

	float speed = mAverageSpeed;

	if (layerMask & mRoadMask) {
		speed = mMaxSpeed;
	} else if (layerMask & mForestMask) {
		speed = mAverageSpeed/2;
	}

	return speed;
}
