
// mapmotionmodel.h: Declaration of the MapMotionModel class.
//
//////////////////////////////////////////////////////////////////////

#ifndef DELTA_MAP_MOTION_MODEL
#define DELTA_MAP_MOTION_MODEL

#include "../common.h"

#include <dtCore/inputdevice.h>
#include <dtCore/motionmodel.h>
#include <dtCore/genericmouselistener.h>
#include <dtCore/isector.h>
#include <dtCore/scene.h>
#include <dtTerrain/terrain.h>

namespace dtCore
{
   class ButtonAxisToAxis;
   class ButtonsToAxis;
   class Keyboard;
   class Mouse;
   class LogicalAxis;
   class LogicalInputDevice;
}

class HTApplication;

/**
* A motion model that causes its target to orbit around a point
* (initially its local origin).
* Supply a valid Keyboard and Mouse to the constructor to setup the default
* control mappings.  Otherwise pass valid instances of Axis to setup the
* MotionModel your own way.
*
* Enable the OrbitMotionModel by calling MotionModel::SetEnabled().  Give the
* OrbitMotionModel something to control by calling SetTarget().
*
* Typical usage:
* \code
* dtCore::dtCore::RefPtr<OrbitMotionModel> orbit = new dtCore::OrbitMotionModel(GetKeyboard(), GetMouse());
* orbit->SetTarget( GetCamera() );
* \endcode
*
* TODO: Not allow camera below terrain
* TODO: Not allow camera beyond the bounds of the terrain
* TODO: Animate zooming in and out
*
*/
class MapMotionModel : public dtCore::MotionModel,
										public dtCore::AxisListener
{
	//DECLARE_MANAGEMENT_LAYER(MapMotionModel)

	public:

		/**
		* Constructor.
		*
		* @param keyboard the keyboard instance, or 0 to
		* avoid creating default input mappings
		* @param mouse the mouse instance, or 0 to avoid
		* creating default input mappings
		*/
		MapMotionModel(HTApplication* htapp = 0, dtCore::Keyboard* keyboard = 0, dtCore::Mouse* mouse = 0, dtCore::Scene * scene = 0);

	protected:

		/**
		* Destructor.
		*/
		virtual ~MapMotionModel();

	public:

		void init();

		/**
		* Sets the input axes to a set of default mappings for mouse
		* and keyboard.
		*
		* @param keyboard the keyboard instance
		* @param mouse the mouse instance
		*/
		void SetDefaultMappings(dtCore::Keyboard* keyboard, dtCore::Mouse* mouse);

		/**
		* Sets the axis that affects the azimuth of the orbit.
		*
		* @param azimuthAxis the new azimuth axis
		*/
		void SetAzimuthAxis(dtCore::Axis* azimuthAxis);

		/**
		* Returns the axis that affects the azimuth of the orbit.
		*
		* @return the current azimuth axis
		*/
		dtCore::Axis* GetAzimuthAxis();

		/**
		* Sets the axis that affects the elevation of the orbit.
		*
		* @param elevationAxis the new elevation axis
		*/
		void SetElevationAxis(dtCore::Axis* elevationAxis);

		/**
		* Returns the axis that affects the elevation of the orbit.
		*
		* @return the current elevation axis
		*/
		dtCore::Axis* GetElevationAxis();

		/**
		* Sets the axis that affects the left/right translation of the orbit.
		*
		* @param leftRightTranslationAxis the new left/right translation axis
		*/
		void SetLeftRightTranslationAxis(dtCore::Axis* leftRightTranslationAxis);

		/**
		* Returns the axis that affects the left/right translation of the orbit.
		*
		* @return the current left/right translation axis
		*/
		dtCore::Axis* GetLeftRightTranslationAxis();

		/**
		* Sets the axis that affects the up/down translation of the orbit.
		*
		* @param upDownTranslationAxis the new up/down translation axis
		*/
		void SetUpDownTranslationAxis(dtCore::Axis* upDownTranslationAxis);

		/**
		* Returns the axis that affects the up/down translation of the orbit.
		*
		* @return the current up/down translation axis
		*/
		dtCore::Axis* GetUpDownTranslationAxis();

		/**
		* Sets the angular rate (the ratio between axis units and angular
		* movement in degrees).
		*
		* @param angularRate the new angular rate
		*/
		void SetAngularRate(float angularRate);

		/**
		* Returns the angular rate.
		*
		* @return the current angular rate
		*/
		float GetAngularRate();

		/**
		* Sets the linear rate (the ratio between axis units and linear
		* movement in meters).
		*
		* @param linearRate the new linear rate
		*/
		void SetLinearRate(float linearRate);

		/**
		* Returns the linear rate.
		*
		* @return the current linear rate
		*/
		float GetLinearRate();

		/**
		* Sets the distance from the focal point.
		*
		* @param distance the new distance
		*/
		void SetDistance(float distance);

		/**
		* Returns the distance from the focal point.
		*
		* @return the current distance
		*/
		float GetDistance();

		/**
		* Called when an axis' state has changed.
		*
		* @param axis the changed axis
		* @param oldState the old state of the axis
		* @param newState the new state of the axis
		* @param delta a delta value indicating stateless motion
		* @return If the
		*/
		virtual bool AxisStateChanged(const dtCore::Axis* axis,
									double oldState,
									double newState,
									double delta);

		bool HandleMouseScrolled(const dtCore::Mouse* mouse, int delta);

		//Updates mDistance
		void update();

	private:

		/**
		 * Updates the xyz and hpr according to the delta on the axis
		 */
		void updateTransform(osg::Vec3& xyz, osg::Vec3& hpr, const dtCore::Axis* axis, double delta);

		/**
		 * Calls updateTransform whilst decaying the velocity param
		 */
		void updateVelocity(osg::Vec3& xyz, osg::Vec3& hpr, const dtCore::Axis* axis, double& vel);

		/**
		 * Updates xyz with the zoom delta
		 */
		void updateZoom(osg::Vec3& xyz, osg::Vec3& hpr, double delta);

		/**
		 * Finds where the camera is looking at
		 */
		osg::Vec3 hitTerrain();

		/**
		* The default input device.
		*/
		dtCore::RefPtr<dtCore::LogicalInputDevice> mDefaultInputDevice;

		/**
		* The left button up/down mapping.
		*/
		dtCore::RefPtr<dtCore::ButtonAxisToAxis> mLeftButtonUpDownMapping;

		/**
		* The left button right/left mapping.
		*/
		dtCore::RefPtr<dtCore::ButtonAxisToAxis> mLeftButtonLeftRightMapping;

		/**
		* The right button up/down mapping.
		*/
		dtCore::RefPtr<dtCore::ButtonAxisToAxis> mRightButtonUpDownMapping;

		/**
		* The right button left/right mapping.
		*/
		dtCore::RefPtr<dtCore::ButtonAxisToAxis> mRightButtonLeftRightMapping;

		/**
		* The default azimuth axis.
		*/
		dtCore::RefPtr<dtCore::LogicalAxis> mDefaultAzimuthAxis;

		/**
		* The default azimuth axis.
		*/
		dtCore::RefPtr<dtCore::LogicalAxis> mDefaultElevationAxis;

		/**
		* The default azimuth axis.
		*/
		dtCore::RefPtr<dtCore::LogicalAxis> mDefaultLeftRightTranslationAxis;

		/**
		* The default azimuth axis.
		*/
		dtCore::RefPtr<dtCore::LogicalAxis> mDefaultUpDownTranslationAxis;

		/**
		* The axis that affects the azimuth of the orbit.
		*/
		dtCore::RefPtr<dtCore::Axis> mAzimuthAxis;

		/**
		* The axis that affects the elevation of the orbit.
		*/
		dtCore::RefPtr<dtCore::Axis> mElevationAxis;

		/**
		* The axis that affects the distance of the orbit.
		*/
		dtCore::RefPtr<dtCore::Axis> mDistanceAxis;

		/**
		* The axis that affects the left/right translation of the orbit.
		*/
		dtCore::RefPtr<dtCore::Axis> mLeftRightTranslationAxis;

		/**
		* The axis that affects the up/down translation of the orbit.
		*/
		dtCore::RefPtr<dtCore::Axis> mUpDownTranslationAxis;

		/**
		* The angular rate (ratio between axis units and angular movement).
		*/
		float mAngularRate;

		/**
		* The linear rate (ratio between axis units and linear movement).
		*/
		float mLinearRate;

		/**
		* The current distance from the focal point.
		*/
		float mDistance;

		dtCore::RefPtr<dtCore::GenericMouseListener> mMouseListener;

		dtCore::Scene* mScene;

		HTApplication* mApp;

		//Keep a short history of deltas to calculate a velocity
		std::deque<double> mAzimuthDeltas;
		std::deque<double> mElevationDeltas;
		std::deque<double> mLeftRightTranslationDeltas;
		std::deque<double> mUpDownTranslationDeltas;

		double mAzimuthVel;
		double mElevationVel;
		double mLeftRightVel;
		double mUpDownVel;
		double mDistanceVel;
		double mDistanceTime;

		bool mUpdateDist;
};

#endif // DELTA_MAP_MOTION_MODEL
