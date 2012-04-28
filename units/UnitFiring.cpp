#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wextra"

#include <dtUtil/boundingshapeutils.h>
#include <dtCore/transform.h>

#pragma GCC diagnostic warning "-Wextra"

#include "../common.h"
#include "UnitFiring.h"
#include "../HTapp.h"
#include "../eventlog.h"

UnitFiring::UnitFiring(const std::string& name, osg::Vec3 position,
		   double heading, UnitSide side, bool controlled, bool remote, bool flying, int client, int id)
: Unit(name, position, heading, side, controlled, remote, flying, client, id),
  m_FiredTime(-std::numeric_limits<double>::max()),
  mAimReset(false),
  mTargetAquired(false),
  //defaults TODO load from external file
  m_ReloadTime(10),
  mProjectileMass(1),
  mProjectileRadius(0.5),
  mProjectileVelocity(500),
  mProjectileYield(1000),
  mProjectileRange2(9000000),
  mAutoFire(true)
  {

  }

/*UnitFiring::UnitFiring(UnitFiring& u) :
	Unit(u),
	m_FiredTime(u.m_FiredTime),
	mAimReset(false),
	mTargetAquired(false),
	m_ReloadTime(u.m_ReloadTime),
	mProjectileMass(u.mProjectileMass),
	mProjectileRadius(u.mProjectileRadius),
	mProjectileVelocity(u.mProjectileVelocity),
	mProjectileYield(u.mProjectileYield),
	mProjectileRange2(u.mProjectileRange2),
	mProjectileType(u.mProjectileType)
{

}*/

UnitFiring::~UnitFiring()
{
	if (m_Initialized) {
		theApp->displayUnitReload(this, INFINITY);
	}
}

UnitFiring* UnitFiring::clone() {
	return new UnitFiring(*this);
}

void UnitFiring::update()
{
	if (mAutoFire) autoFire();
	Unit::update();
}

bool UnitFiring::applyConfigData(const UnitConfigData& data) {
	return (applyUnitConfigData(data) && applyUnitFiringConfigData(data));
}

bool UnitFiring::applyUnitFiringConfigData(const UnitConfigData& data) {
	m_ReloadTime = data.firing.reload;

	mProjectileMass = data.projectile.mass;
	mProjectileRadius = data.projectile.radius;
	mProjectileVelocity = data.projectile.velocity;
	mProjectileYield = data.projectile.yield;
	mProjectileRange2 = data.projectile.range * data.projectile.range;

	if (data.projectile.type == Projectile::SHELL_TEXT) {
		mProjectileType = Projectile::SHELL;
	} else if (data.projectile.type == Projectile::BULLET_TEXT) {
		mProjectileType = Projectile::BULLET;
	} else {
		mProjectileType = Projectile::BULLET;
	}

	return true;
}

bool UnitFiring::launch(osg::Vec3 from, osg::Vec3 direction) {
	bool loaded = isLoaded();
	bool inRange = isTargetInRange(mTargetPosition);
	if (loaded && inRange) {
		//create and launch projectile
		Projectile *projectile = new Projectile(mProjectileRadius, mProjectileMass, mProjectileYield, mProjectileType);
		projectile->init();
		projectile->setPosition(from);
		projectile->setTarget(mTargetPosition);
		projectile->setOwner(this);
		projectile->launch(direction, mProjectileVelocity);

		theApp->BroadcastEvent(Event::FIRE, this,  const_cast<Unit*>(mTargetUnit));

		/*//debug
		osg::Vec3 t = p->getPosition();
		std::cout << "Launching projectile from " << t[0] << "," << t[1] << "," << t[2] << std::endl;
		std::cout << "With direction " << direction[0] << "," << direction[1] << "," << direction[2] << std::endl;
		direction.normalize();
		std::cout << "With (normalized) direction " << direction[0] << "," << direction[1] << "," << direction[2] << std::endl;*/

		reload();

		return true;

	} else if (!loaded) {
		//std::cout << "Reloading! You must wait " << getRemainingReloadTime() << "secs!" << std::endl;
		//theApp->hudGUI->setReload(mReloadWindow, getRemainingReloadTime());
	} else if (!inRange) {
		//std::cout << "Target is out of range! Target is " << (mTargetPosition - getPosition()).length() << " away, max range is " << getProjectileRange() << std::endl;
	}
	return false;
}

bool UnitFiring::getFiringAngle(osg::Vec3 target, float & angle) {
	//constant projectile velocity
	float v = mProjectileVelocity;
	float v2 = v*v;
	float g = -theApp->getGravity();
	//starting pos
	osg::Vec3 p = getPosition();
	//delta pos
	osg::Vec3 dp = target - p;
	//range
	float r2 = dp[0]*dp[0] + dp[1]*dp[1];
	float r = sqrt(r2);

	//for equation see http://en.wikipedia.org/wiki/Trajectory_of_a_projectile
	//(specifically http://upload.wikimedia.org/math/9/8/9/989f9cb1987b7736e916f87b8d9b2806.png)
        float tmp = v2*v2 - g*(g*r2 + 2*dp[2]*v2);
        if (tmp < 0)
          {
            angle = 0;
            return false;
          }
        float A = sqrt(tmp);
	float B = g*r;
	float angle1 = atan((v2 + A)/B);
	float angle2 = atan((v2 - A)/B);

	//if (GetName()=="BlueTank0") std::cout << "Angle:" << angle1 << ',' << angle2 << std::endl;
	//choose the sensible angle
	if (angle1 < angle2 && angle1 > 0) {
		angle = angle1;
	} else {
		angle = angle2;
	}
	return true;
}

bool UnitFiring::getFiringPitchToPosition(osg::Vec3 target, float & angle)
  {
    angle = 0;

    //modify due to unit's pitch and roll
    float pitch = getPitchToPosition(target);
    if (pitch > 0 || pitch < 0) //why doesn't pitch != NAN work?
      {
        angle = pitch;
      }

    float firing_angle;
    if (getFiringAngle(target, firing_angle))
      {
        angle += firing_angle;
      }
    else
      {
        return false;
      }
    return true;
}

bool UnitFiring::isLoaded() {
	return (theApp->getSimTime() >= (m_FiredTime + m_ReloadTime));
}

void UnitFiring::reload() {
	m_FiredTime = theApp->getSimTime();
}

double UnitFiring::getRemainingReloadTime() const {
	return (m_FiredTime + m_ReloadTime) - theApp->getSimTime();
}

bool UnitFiring::isTargetInRange(osg::Vec3 target) {
	return ((target - getPosition()).length2() <= mProjectileRange2);
}

bool UnitFiring::selectTarget()
{
	std::set<Unit*> added_units, removed_units, old_valid_units;
	std::set<Unit*>::iterator vis_it;

	// get the list of all visible targets
	std::set<Unit*> visible_targets = theApp->SearchVisibleTargets(*this);

	// get the list of all units
	std::set<Unit*> all_units = theApp->GetAllUnits();

	// now remove deleted units from the old visible target list
	// (e.g. some units may be deleted)
	set_intersection(mVisibleUnits.begin(),mVisibleUnits.end(),
					 all_units.begin(),all_units.end(),
					 std::inserter(old_valid_units, old_valid_units.end()));

	// now compare to the old set

	// find elements which are in the new list, but not in the old
	std::set_difference(visible_targets.begin(),visible_targets.end(),
				   old_valid_units.begin(),old_valid_units.end(),
				   std::inserter(added_units, added_units.end()));

	// log new units
	for (vis_it = added_units.begin(); vis_it != added_units.end(); vis_it++)
	{
		//only log once per interval
		if (theApp->getSimTime() - EventLog::GetInstance().getLogInterval() > mSeesUnitsTime[*vis_it]) {
			theApp->BroadcastEvent(Event::SEES, this, *vis_it);
			mSeesUnitsTime[*vis_it] = theApp->getSimTime();
		}
	}

	// find elements which are in the old list, but not in the new
	std::set_difference(old_valid_units.begin(),old_valid_units.end(),
				   		visible_targets.begin(),visible_targets.end(),
						std::inserter(removed_units, removed_units.end()));

	// log old units
	for (vis_it = removed_units.begin(); vis_it != removed_units.end(); vis_it++)
	{
		//only log once per interval
		if (theApp->getSimTime() - EventLog::GetInstance().getLogInterval() > mHideUnitsTime[*vis_it]) {
			theApp->BroadcastEvent(Event::HIDE, this, *vis_it);
			mHideUnitsTime[*vis_it] = theApp->getSimTime();
		}
	}

	// now overwrite the old list with the new
	mVisibleUnits.swap(visible_targets);

	// find the nearest visible unit
	osg::Vec3 result, target;
	Unit* unit = 0;
	float distance;
	float nearest = INFINITY;

	for (vis_it = mVisibleUnits.begin(); vis_it != mVisibleUnits.end(); vis_it++)
	{
		//select closest target rather than just the first one!
		target = (*vis_it)->getPosition();
		double height;
		theApp->GetTerrainHeight(target.x(), target.y(), height);
		target.z() = height;
		distance = (target - getPosition()).length2();
		if (distance < nearest)
		{
			result = target;
			unit = *vis_it;
			nearest = distance;
			mAimReset = false;
		}
	}

/*	unsigned i = 0;
	for (vis_it = added_units.begin(); vis_it != added_units.end(); vis_it++)
	{
		std::cout << getName() << " additionally sees " << i << " : " << (*vis_it)->GetName() << std::endl;
		i++;
	}
	i = 0;
	for (vis_it = removed_units.begin(); vis_it != removed_units.end(); vis_it++)
	{
		std::cout << getName() << " hides " << i << " : " << (*vis_it)->GetName() << std::endl;
		i++;
	}
*/

	mTargetPosition = result;
	mTargetUnit = unit;
	return (nearest != INFINITY);
}

osg::Vec3 UnitFiring::calcFireDirection(float headingAngle, float pitchAngle) {
	dtCore::Transform transform;
	osg::Matrix mat;
	osg::Quat q;
	//convert aim heading and pitch to direction

	if (GetNumChildren() == 0) return osg::Vec3(NAN, NAN, NAN);
	dynamic_cast<dtCore::Transformable*>(GetChild(0))->GetTransform(transform);
	transform.GetRotation(mat);
	mat.get(q);
	osg::Vec3 unitDir = q*osg::Vec3(0,1,0);
	osg::Vec3 unitNorm = q*osg::Vec3(0,0,1);

	//rotate Unit's direction to the aim heading about the normal
	osg::Matrixf rotateHeading = osg::Matrixf::rotate(headingAngle, unitNorm);
	osg::Vec3 aimHeading = rotateHeading * unitDir;

	//rotate heading by the aim pitch about normal X heading
	osg::Matrixf rotatePitch = osg::Matrixf::rotate(pitchAngle, unitNorm ^ aimHeading);
	osg::Vec3 aim = rotatePitch * aimHeading;
	return aim;
}

bool UnitFiring::fire() {
	return launch(getProjectileLaunchPos(), calcFireDirection(mAimHeading, mAimPitch));
}

void UnitFiring::autoFire() {
	mTargetAquired = selectTarget();
	if (!mTargetAquired) mAimReset = true;

	if (mTargetAquired) {
		mTargetPosition = predictTargetPosition( const_cast<Unit*>(mTargetUnit));
		aim();
		if (isAimed()) {
			fire();
		}
	} else if (mAimReset) {
		resetAim();
	} else {
		freezeAim();
	}

	theApp->displayUnitReload(this, getRemainingReloadTime());
}

void UnitFiring::calcAimParams() {
        //calc target heading angle
        mAimHeading = getHeadingToPosition(mTargetPosition);
        // calc target pitch angle - might fail because target is too far for the
        // projectile to hit. Then reset aim.
        if (!getFiringPitchToPosition(mTargetPosition, mAimPitch))
          {
              mAimReset = true;
          }
        else
          {
              mAimReset = false;
          }
}

void UnitFiring::aim() {
	calcAimParams();
}

bool UnitFiring::isAimed() {
	return true;
}

osg::Vec3 UnitFiring::predictTargetPosition(Unit *unit) {
	osg::Vec3 currentTargetPos = theApp->mapPointToTerrain(unit->getPosition());
	osg::Vec3 currentTargetVel = unit->getVelocity();

	//calcualte approx time projectile will be in the air
	osg::Vec3 distToTarget = currentTargetPos - getPosition();
        float firing_angle;
        double timeToTarget;
        if (getFiringAngle(currentTargetPos, firing_angle))
          {
            timeToTarget = distToTarget.length() / (mProjectileVelocity * cos(firing_angle));
          }
        else
          {
            timeToTarget = distToTarget.length() / mProjectileVelocity;
          }

	//update position based on what part of the unit can be seen

	//get launch position of projectile
	osg::Vec3 from_pos = getProjectileLaunchPos();

	//find out if the corners of the bounding box of the target are visible
	dtUtil::BoundingBoxVisitor bbv;
	unit->GetChild(0)->GetOSGNode()->accept(bbv);
	//get the 4 top corners Z,Y,X (100,101,110,111 = 4:7)
	osg::Vec3 averageTargetPos(0,0,0);
	int found = 0;
	for (int i=4; i<=7; i++) {
		osg::Vec3 target = bbv.mBoundingBox.corner(i);
		//line of sight
		if (theApp->getTerrain()->IsClearLineOfSight( from_pos, target ) )
		{
			averageTargetPos += target;
			found++;
		}
	}
	averageTargetPos /= found; //aim for the middle of the points we can see in the x&y-axes
	averageTargetPos[2] = currentTargetPos[2]; //aim for the ground in the z-axis

	//update target
	osg::Vec3 predictedPos = averageTargetPos + (currentTargetVel * timeToTarget);

	return predictedPos;
}

osg::Vec3 UnitFiring::getProjectileLaunchPos() {
	return getPosition();
}
