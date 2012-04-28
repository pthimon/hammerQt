#ifndef TERRAIN_PGM_H_
#define TERRAIN_PGM_H_

//////////////////////////////////////////////////////////////////////

#include <set>
#include <dtCore/transformable.h>
#include <dtUtil/noiseutility.h>
#include <dtTerrain/heightfield.h>
#include <osg/Vec3>
#include <ode/collision.h>

#include "../common.h"

class PGMTerrainCallback;
   
/**
* An terrain surface created from a PGM file.
*/
class DT_CORE_EXPORT PGMTerrain : public dtCore::Transformable                                     
{
	friend class PGMTerrainCallback;
      
	DECLARE_MANAGEMENT_LAYER(PGMTerrain)

public:

     /**
      * Constructor.
      *
      * @param name the instance name
      * @param textureImage An image to apply to the terrain
      */
     PGMTerrain(const std::string& name = "PGMTerrain", osg::Image* textureImage = 0);

protected:

     /**
      * Destructor.
      */
     virtual ~PGMTerrain();
  
public:

     /**
      * Load a PGM file.
      */
     bool Load(const std::string & filename);
  
     /**
      * Regenerates the terrain surface.
      */
     void Regenerate();

	/**
	 * Returns the terrain's bounding box.
	 */
   	 osg::BoundingBox GetBB();         
     
     /**
      * Sets the size of the terrain segments. (def  = 1000.0)
      *
      * @param segmentSize the new segment size
      */
     void SetSegmentSize(float segmentSize);
     
     /**
      * Returns the size of the terrain segments.
      *
      * @return the current segment size
      */
     float GetSegmentSize() const;
     
     /**
      * Sets the number of divisions in each segment. (def = 128)
      *
      * @param segmentDivisions the new segment divisions
      */
     void SetSegmentDivisions(int segmentDivisions);
     
     /**
      * Returns the number of divisions in each segment.
      *
      * @return the current segment divisions
      */
     int GetSegmentDivisions() const;
     
     /**
      * Sets the horizontal size for the terrain. The terrain will be centered at (0,0) and
      * be sized as (horizontalSize x horizontalSize) in X and Y dimensions.
      *
      * @param horizontalScale the new horizontal scale
      */
     void SetHorizontalSize(float horizontalSize);
     
     /**
      * Returns the horizontal size.
      *
      * @return the horizontal size
      */
     float GetHorizontalSize() const;

     /**
      * Sets the horizontal scale, which affects the
      * feature frequency. (def = 0.01)
      *
      * @param horizontalScale the new horizontal scale
      */
     void SetHorizontalScale(float horizontalScale);
     
     /**
      * Returns the horizontal scale.
      *
      * @return the horizontal scale
      */
     float GetHorizontalScale() const;
     
     /**
      * Sets the vertical scale, which affects the feature
      * amplitude. (def = 25.0) 
      *
      * @param verticalScale the new vertical scale
      */
     void SetVerticalScale(float verticalScale);
     
     /**
      * Returns the vertical scale.
      *
      * @return the vertical scale
      */
     float GetVerticalScale() const;

     /**
      * Sets the build distance: the distance from the eyepoint for
      * which terrain is guaranteed to be generated. (def = 3000.0)
      *
      * @param buildDistance the new build distance
      */
     void SetBuildDistance(float buildDistance);
     
     /**
      * Returns the build distance.
      *
      * @return the build distance
      */
     float GetBuildDistance() const;

     /**
      * Enables or disables smooth collision detection (collision detection
      * based on the underlying noise function, rather than the triangle
      * mesh).
      *
      * @param enable true to enable, false to disable
      */
     void EnableSmoothCollisions(bool enable);
     
     /**
      * Checks whether smooth collision detection is enabled.
      *
      * @return true if enabled, false if disabled
      */
     bool SmoothCollisionsEnabled() const;
     
     /**
      * Determines the height of the terrain at the specified location.
      *
      * @param x the x coordinate to check
      * @param y the y coordinate to check
      * @param smooth if true, use height of underlying noise function
      * instead of triangle mesh height
      * @return the height at the specified location
      */
     float GetHeight(float x, float y, bool smooth = true);
     
     /**
      * Retrieves the normal of the terrain at the specified location.
      *
      * @param x the x coordinate to check
      * @param y the y coordinate to check
      * @param normal the location at which to store the normal
      * @param smooth if true, use height of underlying noise function
      * instead of triangle mesh height
      */
     void GetNormal(float x, float y, osg::Vec3& normal, bool smooth = true);

     /**
      * We want we collision to happen by default with this class.
      */
     virtual bool FilterContact( dContact* contact, dtCore::Transformable* collider ) { return true; }

    /**
      * Given pointOne and pointTwo, both in world space and with all coordinates
      * in Cartesian space (essentially in meters along X, Y and Z),
      * returns true if there is a clear line of sight and false if the view
      * is blocked.
      * @note This method only samples points at a predetermined distance from
      *       pointOne to pointTwo and not be 100% accurate with actual terrrain.
      *
      * @param pointOne The start point.
      * @param pointTwo The end point.
      * @return Returns true if there is a clear line of sight from pointOne to
      * pointTwo and false if the view is blocked.
      * @see SetLineOfSightSpacing()
      */
     bool IsClearLineOfSight( const osg::Vec3& pointOne,
                              const osg::Vec3& pointTwo );

     /**
       * Set the stepping distance to sample points for the line of sight calculation (meters).
       * Defaults to 25 meters.
       */
     void SetLineOfSightSpacing(float spacing) {mLOSPostSpacing = spacing;}

     float GetLineOfSightSpacing() const {return mLOSPostSpacing;}
     
     dtTerrain::HeightField *GetHeightField() { return m_HeightField.get(); }

  private:

     /**
      * Builds a single terrain segment.
      *
      * @param x the x coordinate at which to build the segment
      * @param y the y coordinate at which to build the segment
      */
     void BuildSegment(int x, int y);

     /**
      * ODE collision function: Gets the contact points between two
      * geoms.
      *
      * @param o1 the first (PGMTerrain) geom
      * @param o2 the second geom
      * @param flags collision flags
      * @param contact the array of contact geoms to fill
      * @param skip the distance between contact geoms in the array
      * @return the number of contact points generated
      */
     static int Collider(dGeomID o1, dGeomID o2, int flags,
                         dContactGeom* contact, int skip);

     /**
     * A Helper function for Collider to detect collision with terrain and a sphere
     *  
     * @param it the the pointer to the PGM terrain we want to collide with
     * @param pCenter the center of the sphere
     * @param pRadius the radius of the sphere
     * @param pContact the ode contact point to fill 
     * @return whether or not a collision occured
     */
     static bool CollideSphere(PGMTerrain* it, const osg::Vec3& pCenter, float pRadius, dContactGeom* pContact);
     
     /**
      * ODE collision function: Finds the collider function appropriate
      * to detect collisions between PGMTerrain geoms and other
      * geoms.
      *
      * @param num the class number of the geom class to check
      * @return the appropriate collider function, or NULL for none
      */
     static dColliderFn* GetColliderFn(int num);

     
     /**
      * ODE collision function: Computes the axis-aligned bounding box
      * for PGMTerrain instances.
      *
      * @param g the geom to check
      * @param aabb the location in which to store the axis-aligned
      * bounding box
      */
     static void GetAABB(dGeomID g, dReal aabb[6]);
     
     /**
      * ODE collision function: Checks whether the specified axis-aligned
      * bounding box intersects with an PGMTerrain instance.
      *
      * @param o1 the first (PGMTerrain) geom
      * @param o2 the second geom
      * @param aabb2 the axis-aligned bounding box of the second geom
      * @return 1 if it intersects, 0 if it does not
      */
     static int AABBTest(dGeomID o1, dGeomID o2, dReal aabb2[6]);


     //returns an interpolated color based on the height
     osg::Vec4 GetColor(float pHeight);

     //returns a shaded color based on the normal and the height
     osg::Vec4 GetColorAtXY(float x, float y);

     //initializes info used for the GetColor function
     void SetupColorInfo();
     
     /**
      * The noise object.
      */
     dtUtil::Noise2f mNoise;
     
     /**
      * The size of each terrain segment.
      */
     float mSegmentSize;
     
     /**
      * The number of divisions in each terrain segment.
      */
     int mSegmentDivisions;
     
     /**
      * The horizontal scale.
      */
     float mHorizontalScale;
     
     /**
      * The vertical scale.
      */
     float mVerticalScale;
     
     /**
      * The build distance.
      */
     float mBuildDistance;

     /**
      * Whether or not smooth collision detection is enabled.
      */
     bool mSmoothCollisionsEnabled;
     
     /**
      * Identifies a single terrain segment.
      */
     struct Segment
     {
        int mX, mY;
        
        Segment(int x, int y)
           : mX(x), mY(y)
        {}
        
        bool operator<(const Segment& segment) const
        {
           if(mX < segment.mX) return true;
           else if(mX > segment.mX) return false;
           else return (mY < segment.mY);
        }
     };
     
     /**
      * The set of constructed segments.
      */
     std::set<Segment> mBuiltSegments;
     
     /**
      * Flags the segments as needing to be cleared.
      */
     bool mClearFlag;

     //added for vertex coloring
     float mMinHeight, mIdealHeight, mMaxHeight;
     float mMinColorIncrement, mMaxColorIncrement;
     osg::Vec3 mMinColor, mIdealColor, mMaxColor;

     float mLOSPostSpacing; ///<used to samples points for LOS calculations
        	 
	 // internal parameters for loading the .pgm
     double m_foldingBoundary;
     
     // an array storing the heightfield
     dtCore::RefPtr<dtTerrain::HeightField> m_HeightField;
     
     float m_HorizontalSize;
     
     short m_HFMaxValue, m_HFMinValue;
   };

#endif // TERRAIN_PGM_H_
