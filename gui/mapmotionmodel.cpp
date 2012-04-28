// orbitmotionmodel.cpp: Implementation of the MapMotionModel class.
//
//////////////////////////////////////////////////////////////////////

#include "mapmotionmodel.h"
#include "../HTapp.h"

#include <numeric>

#include <dtCore/keyboard.h>
#include <dtCore/mouse.h>
#include <dtCore/logicalinputdevice.h>
#include <dtCore/scene.h>
#include <dtCore/motionmodel.h>
#include <dtCore/transformable.h>
#include <dtCore/transform.h>
#include <dtUtil/matrixutil.h>

#include <osg/Vec3>
#include <osg/Matrix>

//namespace dtCore
//{

//IMPLEMENT_MANAGEMENT_LAYER(MapMotionModel)

/**
 * Constructor.
 *
 * @param keyboard the keyboard instance, or 0 to
 * avoid creating default input mappings
 * @param mouse the mouse instance, or 0 to avoid
 * creating default input mappings
 */
MapMotionModel::MapMotionModel(HTApplication *app, dtCore::Keyboard* keyboard,
                                   dtCore::Mouse* mouse, dtCore::Scene* scene)
   : MotionModel("MapMotionModel"),
     mAzimuthAxis(0),
     mElevationAxis(0),
     mDistanceAxis(0),
     mLeftRightTranslationAxis(0),
     mUpDownTranslationAxis(0),
     mAngularRate(90.0f),
     mLinearRate(0.5f),
     mDistance(100.0f),
	 mMouseListener( new dtCore::GenericMouseListener() ),
	mScene(scene),
	mApp(app),
	mUpdateDist(false)
{
   //RegisterInstance(this);

   if(keyboard != 0 && mouse != 0)
   {
      SetDefaultMappings(keyboard, mouse);
   }

	mAzimuthVel = 0;
	mElevationVel = 0;
	mLeftRightVel = 0;
	mUpDownVel = 0;
	mDistanceVel = 0;
	mDistanceTime = 0;
}

/**
 * Destructor.
 */
MapMotionModel::~MapMotionModel()
{
   SetAzimuthAxis(0);
   SetElevationAxis(0);
   //SetDistanceAxis(0);
   SetLeftRightTranslationAxis(0);
   SetUpDownTranslationAxis(0);

   //DeregisterInstance(this);
}

/**
 * Sets the input axes to a set of default mappings for mouse
 * and keyboard.
 *
 * @param keyboard the keyboard instance
 * @param mouse the mouse instance
 */
void MapMotionModel::SetDefaultMappings(dtCore::Keyboard* keyboard, dtCore::Mouse* mouse)
{
	//TODO: Use Mouse/Keyboard listeners rather than axis listeners

   if(!mDefaultInputDevice.valid())
   {
	   mDefaultInputDevice = new dtCore::LogicalInputDevice;

	   mLeftButtonUpDownMapping = new dtCore::ButtonAxisToAxis(   mouse->GetButton(dtCore::Mouse::MiddleButton),
                                                         mouse->GetAxis(1) );
      mDefaultElevationAxis = mDefaultInputDevice->AddAxis( "left mouse button up/down",
                                                            mLeftButtonUpDownMapping.get() );

	  mLeftButtonLeftRightMapping = new dtCore::ButtonAxisToAxis(   mouse->GetButton(dtCore::Mouse::MiddleButton),
                                                            mouse->GetAxis(0) );
      mDefaultAzimuthAxis = mDefaultInputDevice->AddAxis(   "left mouse button left/right",
                                                            mLeftButtonLeftRightMapping.get() );

	  mRightButtonUpDownMapping = new dtCore::ButtonAxisToAxis(  mouse->GetButton(dtCore::Mouse::RightButton),
                                                         mouse->GetAxis(1) );
	  mDefaultUpDownTranslationAxis = mDefaultInputDevice->AddAxis(  "right mouse button forward/back",
                                                                     mRightButtonUpDownMapping.get() );

	  mRightButtonLeftRightMapping = new dtCore::ButtonAxisToAxis(  mouse->GetButton(dtCore::Mouse::RightButton),
                                                            mouse->GetAxis(0) );
      mDefaultLeftRightTranslationAxis = mDefaultInputDevice->AddAxis(  "right mouse button left/right",
                                                                        mRightButtonLeftRightMapping.get() );
   }
   else
   {
	   mLeftButtonUpDownMapping->SetSourceButton(mouse->GetButton(dtCore::Mouse::MiddleButton));
      mLeftButtonUpDownMapping->SetSourceAxis(mouse->GetAxis(1));

	  mLeftButtonLeftRightMapping->SetSourceButton(mouse->GetButton(dtCore::Mouse::MiddleButton));
      mLeftButtonLeftRightMapping->SetSourceAxis(mouse->GetAxis(0));

	  mRightButtonUpDownMapping->SetSourceButton(mouse->GetButton(dtCore::Mouse::RightButton));
      mRightButtonUpDownMapping->SetSourceAxis(mouse->GetAxis(1));

	  mRightButtonLeftRightMapping->SetSourceButton(mouse->GetButton(dtCore::Mouse::RightButton));
      mRightButtonLeftRightMapping->SetSourceAxis(mouse->GetAxis(0));
   }

   SetAzimuthAxis(mDefaultAzimuthAxis.get());

   SetElevationAxis(mDefaultElevationAxis.get());

   SetLeftRightTranslationAxis(mDefaultLeftRightTranslationAxis.get());

   SetUpDownTranslationAxis(mDefaultUpDownTranslationAxis.get());


   mMouseListener->SetScrolledCallback(dtCore::GenericMouseListener::WheelCallbackType(this, &MapMotionModel::HandleMouseScrolled));

   //Make sure subsequent mouse listeners are added to the front of the queue to override this
   mouse->AddMouseListener( mMouseListener.get() );
}

/**
 * Sets the axis that affects the azimuth of the orbit.
 *
 * @param azimuthAxis the new azimuth axis
 */
void MapMotionModel::SetAzimuthAxis(dtCore::Axis* azimuthAxis)
{
   if(mAzimuthAxis != 0)
   {
      mAzimuthAxis->RemoveAxisListener(this);
   }

   mAzimuthAxis = azimuthAxis;

   if(mAzimuthAxis != 0)
   {
      mAzimuthAxis->AddAxisListener(this);
   }
}

/**
 * Returns the axis that affects the azimuth of the orbit.
 *
 * @return the current azimuth axis
 */
dtCore::Axis* MapMotionModel::GetAzimuthAxis()
{
   return mAzimuthAxis.get();
}

/**
 * Sets the axis that affects the elevation of the orbit.
 *
 * @param elevationAxis the new elevation axis
 */
void MapMotionModel::SetElevationAxis(dtCore::Axis* elevationAxis)
{
   if(mElevationAxis != 0)
   {
      mElevationAxis->RemoveAxisListener(this);
   }

   mElevationAxis = elevationAxis;

   if(mElevationAxis != 0)
   {
      mElevationAxis->AddAxisListener(this);
   }
}

/**
 * Returns the axis that affects the elevation of the orbit.
 *
 * @return the current elevation axis
 */
dtCore::Axis* MapMotionModel::GetElevationAxis()
{
   return mElevationAxis.get();
}

/**
 * Sets the axis that affects the left/right translation of the orbit.
 *
 * @param leftRightTranslationAxis the new left/right translation axis
 */
void MapMotionModel::SetLeftRightTranslationAxis(dtCore::Axis* leftRightTranslationAxis)
{
   if(mLeftRightTranslationAxis != 0)
   {
      mLeftRightTranslationAxis->RemoveAxisListener(this);
   }

   mLeftRightTranslationAxis = leftRightTranslationAxis;

   if(mLeftRightTranslationAxis != 0)
   {
      mLeftRightTranslationAxis->AddAxisListener(this);
   }
}

/**
 * Returns the axis that affects the left/right translation of the orbit.
 *
 * @return the current left/right translation axis
 */
dtCore::Axis* MapMotionModel::GetLeftRightTranslationAxis()
{
   return mLeftRightTranslationAxis.get();
}

/**
 * Sets the axis that affects the up/down translation of the orbit.
 *
 * @param upDownTranslationAxis the new up/down translation axis
 */
void MapMotionModel::SetUpDownTranslationAxis(dtCore::Axis* upDownTranslationAxis)
{
   if(mUpDownTranslationAxis != 0)
   {
      mUpDownTranslationAxis->RemoveAxisListener(this);
   }

   mUpDownTranslationAxis = upDownTranslationAxis;

   if(mUpDownTranslationAxis != 0)
   {
      mUpDownTranslationAxis->AddAxisListener(this);
   }
}

/**
 * Returns the axis that affects the up/down translation of the orbit.
 *
 * @return the current up/down translation axis
 */
dtCore::Axis* MapMotionModel::GetUpDownTranslationAxis()
{
   return mUpDownTranslationAxis.get();
}

/**
 * Sets the angular rate (the ratio between axis units and angular
 * movement in degrees).
 *
 * @param angularRate the new angular rate
 */
void MapMotionModel::SetAngularRate(float angularRate)
{
   mAngularRate = angularRate;
}

/**
 * Returns the angular rate.
 *
 * @return the current angular rate
 */
float MapMotionModel::GetAngularRate()
{
   return mAngularRate;
}

/**
 * Sets the linear rate (the ratio between axis units and linear
 * movement in meters).
 *
 * @param linearRate the new linear rate
 */
void MapMotionModel::SetLinearRate(float linearRate)
{
   mLinearRate = linearRate;
}

/**
 * Returns the linear rate.
 *
 * @return the current linear rate
 */
float MapMotionModel::GetLinearRate()
{
   return mLinearRate;
}

/**
 * Sets the distance from the focal point.
 *
 * @param distance the new distance
 */
void MapMotionModel::SetDistance(float distance)
{
   mDistance = distance;
}

/**
 * Returns the distance from the focal point.
 *
 * @return the current distance
 */
float MapMotionModel::GetDistance()
{
   return mDistance;
}

/**
 * Called when an axis' state has changed.
 *
 * @param axis the changed axis
 * @param oldState the old state of the axis
 * @param newState the new state of the axis
 * @param delta a delta value indicating stateless motion
 */
bool MapMotionModel::AxisStateChanged(const dtCore::Axis* axis,
                                        double oldState,
                                        double newState,
                                        double delta)
{
	unsigned int historyLength = 5;
	if (GetTarget() != 0 && IsEnabled()) {
		//Keep track of history of deltas on all 4 axes to calculate a velocity
		//TODO calculate velocities on mouse up
		if (axis == mAzimuthAxis.get()) {
			mAzimuthDeltas.push_back(delta);
			if (mAzimuthDeltas.size() > historyLength) {
				mAzimuthDeltas.pop_front();
			}
			mAzimuthVel = std::accumulate(mAzimuthDeltas.begin(), mAzimuthDeltas.end(), 0.0) / mAzimuthDeltas.size();
		} else if (axis == mElevationAxis.get()) {
			mElevationDeltas.push_back(delta);
			if (mElevationDeltas.size() > historyLength) {
				mElevationDeltas.pop_front();
			}
			mElevationVel = std::accumulate(mElevationDeltas.begin(), mElevationDeltas.end(), 0.0) / mElevationDeltas.size();
		} else if (axis == mLeftRightTranslationAxis.get()) {
			mLeftRightTranslationDeltas.push_back(delta);
			if (mLeftRightTranslationDeltas.size() > historyLength) {
				mLeftRightTranslationDeltas.pop_front();
			}
			mLeftRightVel = std::accumulate(mLeftRightTranslationDeltas.begin(), mLeftRightTranslationDeltas.end(), 0.0) / mLeftRightTranslationDeltas.size();

			mApp->setLookAtUnit(NULL);
		} else if (axis == mUpDownTranslationAxis.get()) {
			mUpDownTranslationDeltas.push_back(delta);
			if (mUpDownTranslationDeltas.size() > historyLength) {
				mUpDownTranslationDeltas.pop_front();
			}
			mUpDownVel = std::accumulate(mUpDownTranslationDeltas.begin(), mUpDownTranslationDeltas.end(), 0.0) / mUpDownTranslationDeltas.size();

			mApp->setLookAtUnit(NULL);
		}

		dtCore::Transform transform;
		GetTarget()->GetTransform(transform);
		osg::Vec3 xyz, hpr;
		transform.Get(xyz, hpr);

		updateTransform(xyz, hpr, axis, delta);

		transform.Set(xyz, hpr);
		GetTarget()->SetTransform(transform);

		return true;
	}

	return false;
}

void MapMotionModel::updateTransform(osg::Vec3& xyz, osg::Vec3& hpr, const dtCore::Axis* axis, double delta) {
	dtCore::Transform transform;
	transform.Set(xyz, hpr);
	osg::Matrix mat;
	//so we can reset the hpr later if the camera goes below the terrain
	osg::Vec3 oldhpr = hpr;

	if (axis == mAzimuthAxis.get()) {
		osg::Vec3 focus(0.0f, mDistance, 0.0f);
		transform.Get(mat);
		dtUtil::MatrixUtil::TransformVec3(focus, mat);
		hpr[0] -= float(delta * mAngularRate);
		osg::Vec3 offset(0.0f, -mDistance, 0.0f);
		dtUtil::MatrixUtil::PositionAndHprToMatrix(mat, focus, hpr);
		dtUtil::MatrixUtil::TransformVec3(xyz, offset, mat);
	} else if (axis == mElevationAxis.get()) {
		osg::Vec3 focus(0.0f, mDistance, 0.0f);
		transform.Get(mat);
		dtUtil::MatrixUtil::TransformVec3(focus, mat);
		hpr[1] += float(delta * mAngularRate);
		if (hpr[1] < -89.9f) {
			hpr[1] = -89.9f;
		} else if (hpr[1] > 89.9f) {
			hpr[1] = 89.9;
		}
		osg::Vec3 offset(0.0f, -mDistance, 0.0f);
		dtUtil::MatrixUtil::PositionAndHprToMatrix(mat, focus, hpr);
		dtUtil::MatrixUtil::TransformVec3(xyz, offset, mat);
	} else if (axis == mLeftRightTranslationAxis.get()) {
		osg::Vec3 translation(-float(delta * mDistance * mLinearRate),
				0.0f, 0.0f);
		osg::Matrix mat;
		dtUtil::MatrixUtil::HprToMatrix(mat, hpr);
		translation = osg::Matrix::transform3x3(translation, mat);
		xyz += translation;
		mUpdateDist = true;
	} else if (axis == mUpDownTranslationAxis.get()) {
		//forward / back
		osg::Vec3 translation(0.0f,
				-float(delta * mDistance * mLinearRate), 0.0f);
		osg::Matrix mat;
		osg::Vec3 heading(hpr[0], 0.0f, 0.0f);
		dtUtil::MatrixUtil::HprToMatrix(mat, heading);
		translation = osg::Matrix::transform3x3(translation, mat);
		xyz += translation;
		mUpdateDist = true;
	}

	//Stop the camera going below the terrain
	double terrainHeight;
	if (mApp->GetTerrainHeight(xyz[0], xyz[1], terrainHeight)) {
		if ((xyz[2]) < terrainHeight + 2) {
			//fix z-coord and pitch
			xyz[2] = terrainHeight + 2;
			//TODO: recalculate heading and pitch properly
			hpr[1] = oldhpr[1];
		}
	}
}

void MapMotionModel::updateZoom(osg::Vec3& xyz, osg::Vec3& hpr, double delta) {
	float distDelta = -float(delta * mDistance * mLinearRate * 0.1);
	//dont get too close!
	if(mDistance + distDelta < 1.0f)
	{
		distDelta = 1.0f - mDistance;
	}
	osg::Vec3 translation ( 0.0f, -distDelta, 0.0f );
	osg::Matrix mat;
	dtUtil::MatrixUtil::HprToMatrix(mat, hpr);
	translation = osg::Matrix::transform3x3(translation, mat);
	xyz += translation;

	mDistance += distDelta;
	theApp->zoomUpdate(mDistance);
}

osg::Vec3 MapMotionModel::hitTerrain() {
	dtCore::Transform transform;
	osg::Vec3 xyz, hpr;
	osg::Vec3 direction, hitPt;
	osg::Matrix mat;
	osg::Quat q;

	GetTarget()->GetTransform(transform);
	transform.Get(xyz, hpr);

	dtCore::Isector *isect = new dtCore::Isector( mScene );
	isect->SetStartPosition( xyz );

	transform.GetRotation(mat);
	mat.get(q);
	direction = q*osg::Vec3(0,1,0);
  	isect->SetDirection( direction );
  	isect->SetLength(100000);
	isect->Update();
	osgUtil::IntersectVisitor::HitList hits = isect->GetHitList();
	if (!hits.empty()) {
		//if many hits - return the one closest to the target
		float hitDist = INFINITY;
		for (unsigned int i=0; i < hits.size(); i++) {
			osg::NodePath nodePath = hits.at(i)._nodePath;
	         for(  osg::NodePath::iterator nodeIter = nodePath.begin();
	               nodeIter != nodePath.end();
	               ++nodeIter )
	         {
	         	if ((*nodeIter)->getName() == theApp->getTerrainName()) {
	         		osg::Vec3 localHitPt = hits.at(i).getWorldIntersectPoint();
	         		float localDist = (xyz - localHitPt).length2();
	         		if (localDist < hitDist) {
	         			//dont move the focal point by more than 100 in one go TODO: fix this
	         			//if (fabs((xyz - localHitPt).length() - mDistance) < 100) {
	         				hitPt = localHitPt;
	         			//}
	         		}
	         	}
	         }
		}
	}
	return hitPt;
}

bool MapMotionModel::HandleMouseScrolled(const dtCore::Mouse* mouse, int delta) {
	if(GetTarget() != 0 && IsEnabled())
	{
		double interval = theApp->getSimTime() - mDistanceTime;
		if (interval > 1) {
			//max interval is 1, hence minimum velocity is delta
			interval = 1;
		} else if (interval < 0.1) {
			//min interval is 0.1, hence max velocity is delta * 10
			interval = 0.1;
		}
		mDistanceVel += delta / interval;
		mDistanceTime = theApp->getSimTime();
	}

	return true;
}

void MapMotionModel::updateVelocity(osg::Vec3& xyz, osg::Vec3& hpr, const dtCore::Axis* axis, double& delta) {
	delta *= 0.8;
	if (fabs(delta) > 0.001) {
		updateTransform(xyz, hpr, axis, delta);
	} else {
		delta = 0;
	}
}

/**
 * Called from HTApplication::PreFrameGUI
 */
void MapMotionModel::update() {
	dtCore::Transform trans;

	GetTarget()->GetTransform(trans);
	osg::Vec3 xyz, hpr;
	trans.Get(xyz, hpr);

	//update zoom
	mDistanceVel *= 0.8;
	if (fabs(mDistanceVel) > 0.1) {
		updateZoom(xyz, hpr, mDistanceVel/15);
	} else {
		mDistanceVel = 0;
	}

	//update other axes
	updateVelocity(xyz, hpr, mAzimuthAxis.get(), mAzimuthVel);
	updateVelocity(xyz, hpr, mElevationAxis.get(), mElevationVel);
	updateVelocity(xyz, hpr, mLeftRightTranslationAxis.get(), mLeftRightVel);
	updateVelocity(xyz, hpr, mUpDownTranslationAxis.get(), mUpDownVel);

	trans.Set(xyz, hpr);
	GetTarget()->SetTransform(trans);

	//update distance to terrain - the point that the camera rotates around
	if (mUpdateDist) {
		mDistance = (xyz - hitTerrain()).length();
		mUpdateDist = false;
	}
}

//}
