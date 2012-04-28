#ifndef SURFACEOBJECT_H_
#define SURFACEOBJECT_H_

//////////////////////////////////////////////////////////////////////

#include <set>
#include <dtCore/transformable.h>
#include <dtCore/object.h>
#include <dtUtil/noiseutility.h>
#include <dtTerrain/heightfield.h>
#include <osg/Vec3>
#include <ode/collision.h>

#include "../common.h"

namespace osgSim {
  class OverlayNode;
}

class SurfaceObjectCallback;

/**
 * A terrain surface created from a heightfield, with collision.
 * Each segment should contain less than 3 different textures,
 * otherwise aliasing problems may arise.
 */
class DT_CORE_EXPORT SurfaceObject : public dtCore::Transformable
  {
    friend class SurfaceObjectCallback;

  DECLARE_MANAGEMENT_LAYER(SurfaceObject)

  public:

  /**
   * Constructor.
   *
   * @param name the instance name
   * @param heightfield The heightfield to use
   * @param textureImage An image to apply to the terrain
   */
  SurfaceObject(dtCore::RefPtr<dtTerrain::HeightField> heightfield,
      dtCore::RefPtr<dtTerrain::HeightField> attribute_map,
      std::vector< dtCore::RefPtr<osg::Texture2D> > texture_array,
      osg::Texture2D* terrain_texture, osg::Texture2D* terrain_mask, osg::Texture2D* terrain_recolor,
      dtCore::Object* terrain_overlay = 0, const std::string& name = "Surface");

  osg::Node* GetOSGNode();
  const osg::Node* GetOSGNode() const;

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
   * Sets the minimum altitude for the heightfield.
   *
   * @param min_alt the new minimum altitude.
   */
  inline void SetMinAlt(float min_alt) { m_MinAlt = min_alt; mClearFlag = true; }

  /**
   * Returns the minimum altitude.
   *
   * @return the minimum altitude.
   */
  inline float GetMinAlt() const { return m_MinAlt; }

  /**
   * Sets the maximum altitude for the heightfield.
   *
   * @param max_alt the new maximum altitude.
   */
  inline void SetMaxAlt(float max_alt) { m_MaxAlt = max_alt; mClearFlag = true; }

  /**
   * Returns the maximum altitude.
   *
   * @return the maximum altitude.
   */
  inline float GetMaxAlt() const { return m_MaxAlt; }

  /**
   * Sets the vertical scale.
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
   * Determines the height of the terrain at the specified location.
   *
   * @param x the x coordinate to check
   * @param y the y coordinate to check
   * @return the height at the specified location
   */
  float GetHeight(float x, float y);

  /**
   * Determines the texture index at the specified location.
   *
   * @param x the x coordinate to check
   * @param y the y coordinate to check
   * @return the index at the specified location
   */
  unsigned int GetTextureIndex(float x, float y);

  /**
   * Retrieves the normal of the terrain at the specified location.
   *
   * @param x the x coordinate to check
   * @param y the y coordinate to check
   * @param normal the location at which to store the normal
   */
  void GetNormal(float x, float y, osg::Vec3& normal);

  /**
   * We want we collision to happen by default with this class.
   */
  virtual bool FilterContact( dContact* contact, dtCore::Transformable* collider )
    { return true;}

  protected:

  /**
   * Destructor.
   */
  virtual ~SurfaceObject();

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
   * @param o1 the first (SurfaceObject) geom
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
  static bool CollideSphere(SurfaceObject* it, const osg::Vec3& pCenter, float pRadius, dContactGeom* pContact);

  /**
   * ODE collision function: Finds the collider function appropriate
   * to detect collisions between SurfaceObject geoms and other
   * geoms.
   *
   * @param num the class number of the geom class to check
   * @return the appropriate collider function, or NULL for none
   */
  static dColliderFn* GetColliderFn(int num);

  /**
   * ODE collision function: Computes the axis-aligned bounding box
   * for SurfaceObject instances.
   *
   * @param g the geom to check
   * @param aabb the location in which to store the axis-aligned
   * bounding box
   */
  static void GetAABB(dGeomID g, dReal aabb[6]);

  /**
   * ODE collision function: Checks whether the specified axis-aligned
   * bounding box intersects with an SurfaceObject instance.
   *
   * @param o1 the first (SurfaceObject) geom
   * @param o2 the second geom
   * @param aabb2 the axis-aligned bounding box of the second geom
   * @return 1 if it intersects, 0 if it does not
   */
  static int AABBTest(dGeomID o1, dGeomID o2, dReal aabb2[6]);

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
   * The size of each terrain segment.
   */
  float mSegmentSize;

  /**
   * The number of divisions in each terrain segment.
   */
  int mSegmentDivisions;

  /**
   * The vertical scale.
   */
  float m_VerticalScale;

  /**
   * The horizontal size.
   */
  float m_HorizontalSize;

  /**
   * The build distance.
   */
  float mBuildDistance;

  /**
   * Flags the segments as needing to be cleared.
   */
  bool mClearFlag;

  //added for vertex coloring
  float mMinHeight, mIdealHeight, mMaxHeight;
  float mMinColorIncrement, mMaxColorIncrement;
  osg::Vec3 mMinColor, mIdealColor, mMaxColor;

  float m_MinAlt, m_MaxAlt;

  float m_HFMaxValue, m_HFMinValue;

  // an array storing the heightfield
  dtCore::RefPtr<dtTerrain::HeightField> m_HeightField;
  // an array storing the texture indices associated with points of the surface
  dtCore::RefPtr<dtTerrain::HeightField> m_AttributeMap;
  // The global texture for the terrain
  dtCore::RefPtr<osg::Texture2D> m_TerrainTexture;
  // The color mask for the terrain
  dtCore::RefPtr<osg::Texture2D> m_TerrainMask;
  // The recolor mask
  dtCore::RefPtr<osg::Texture2D> m_Recolor;

  // stores the detail textures (indices into this array is stored in m_AttributeMap)
  std::vector< dtCore::RefPtr<osg::Texture2D> > m_TextureArray;

  dtCore::RefPtr<osgSim::OverlayNode> m_OverlayNode;
};

#endif // SURFACEOBJECT_H_
