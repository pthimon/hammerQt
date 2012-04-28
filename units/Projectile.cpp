#include <dtCore/transform.h>
#include "Projectile.h"
#include "../HTapp.h"

std::set<dtCore::RefPtr<Projectile> > Projectile::projectiles;

const std::string Projectile::SHELL_TEXT = "shell";
const std::string Projectile::BULLET_TEXT = "bullet";

/**
 * A projectile class
 */
Projectile::Projectile(float radius, float mass, float yield, ProjectileType type): dtCore::Object("projectile")
{
	this->radius = radius;
	this->mass = mass;
	this->mYield = yield;
	this->mType = type;
	armed = false;
	mDetonateTime = 0;
	mHit = false;
}

Projectile::~Projectile()
{
	//delete explosion
	theApp->GetScene()->RemoveDrawable(mExplosion.get());
}

void Projectile::init() {
	//add to projectile list
	Projectile::projectiles.insert(this);

	if (!LoadFile("sphere.osg")) return;
	//dtCore::Transform t;
	//t.SetScale(1,1,1);
	//SetTransform(t);

	//add to scene
	theApp->GetScene()->AddDrawable(this);
	initPhysics();

	//load explosion
	mExplosion = new dtCore::ParticleSystem();
	if (mType == Projectile::SHELL) {
		mExplosion->LoadFile("Particles/explosion_large2.osg",true);
	} else {
		mExplosion->LoadFile("Particles/small_dust.osg",true);
	}
	theApp->GetScene()->AddDrawable(mExplosion.get());
	mExplosion->SetEnabled(false);
}

/**
 * Should be called after the object has been added to the scene
 */
void Projectile::initPhysics() {
	//set mass
	dMassSetSphereTotal (&m,mass,radius);
	SetMass(&m);

	//create geom
	SetCollisionSphere(radius);

	EnableDynamics(true);
}

void Projectile::setPosition(osg::Vec3 pos) {

	dtCore::Transform current_tr;
	GetTransform(current_tr);
	current_tr.SetTranslation( pos );
	SetTransform(current_tr);

	PrePhysicsStepUpdate();
}

osg::Vec3 Projectile::getPosition()
{
	osg::Vec3 center;
	dtCore::Transform current_tr;
	GetTransform(current_tr);
	current_tr.GetTranslation(center);
	return center;
}

void Projectile::launch(osg::Vec3 direction, float force) {
	direction.normalize();
	dBodySetLinearVel (GetBodyID(),force*direction[0],force*direction[1],force*direction[2]);
	dBodySetAngularVel (GetBodyID(),0,0,0);
	//activate
	armed = true;
	mPrevPosition = getPosition();
}

/**
 * Returns true when projectile has exploded
 */
bool Projectile::update() {
	if (armed) {
		double height;
		osg::Vec3 t = getPosition();
		theApp->GetTerrainHeight(t[0], t[1], height);
		//Unit *hit = theApp->PointToUnit(t);
		Unit *hit = theApp->TrajectoryToUnit(mPrevPosition, t);
		if ((hit && hit != mOwner.get()) || t[2] < height) {
			if (hit) {
				//projectile has intersected, so make sure projectile detonates on the unit
				setPosition(hit->getPosition());
			}
			mHit = hit;
			// std::cout << "Projectile hit target at: " << t[0] << "," << t[1] << "," << t[2] << std::endl;
			return true;
		}
		mPrevPosition = t;
	}
	return false;
}

//TODO delete projectile and explosion
void Projectile::detonate() {
	//std::cout << "BOOOM" << std::endl;
	//start explosion
	dtCore::Transform trans;
	trans.SetTranslation(getPosition());
	mExplosion->SetTransform(trans);
    mExplosion->SetEnabled(true);
	//remove from scene
	theApp->GetScene()->RemoveDrawable(this);
	//(TODO fix delete so it doesnt crash!)
	//delete this;
	mDetonateTime = theApp->getSimTime();
}
