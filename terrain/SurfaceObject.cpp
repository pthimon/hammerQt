//////////////////////////////////////////////////////////////////////
#include "../common.h"
#pragma GCC diagnostic ignored "-Wextra"

#include <prefix/dtcoreprefix-src.h>
#include <dtCore/globals.h>
#include <dtCore/light.h>
#include <dtCore/shadermanager.h>
#include <dtCore/shaderparameter.h>
#include <dtCore/object.h>
#include <dtUtil/mathdefines.h>
#include <dtUtil/matrixutil.h>
#include <dtUtil/log.h>
#include <dtTerrain/heightfield.h>
#include <vector>
#include <cmath>
#include "boost/lexical_cast.hpp"

#include <osg/LOD>
#include <osg/Drawable>
#include <osg/Geode>
#include <osg/MatrixTransform>
#include <osg/Geometry>
#include <osg/Plane>
#include <osg/TexEnv>
#include <osg/Texture2D>
#include <osg/PrimitiveSet>
#include <osg/ShadeModel>
#include <osg/LightSource>
#include <osgDB/ReadFile>
#include <osgSim/OverlayNode>

#include "SurfaceObject.h"
#include "../HTapp.h"

#pragma GCC diagnostic warning "-Wextra"

using namespace dtCore;

IMPLEMENT_MANAGEMENT_LAYER(SurfaceObject)

/**
 * The terrain callback class.  Builds terrain segments
 * around viewer.
 */
class SurfaceObjectCallback : public osg::NodeCallback
{
   public:

      /**
       * Constructor.
       *
       * @param terrain the owning SurfaceObject object
       */
      SurfaceObjectCallback(SurfaceObject* terrain)
         : mTerrain(terrain)
      {}

      /**
       * Callback function.
       *
       * @param node the node to operate on
       * @param nv the active node visitor
       */
      virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
      {
         if(mTerrain->mClearFlag)
         {
            mTerrain->GetMatrixNode()->removeChild(0, mTerrain->GetMatrixNode()->getNumChildren());

            mTerrain->mBuiltSegments.clear();

            mTerrain->mClearFlag = false;
         }

         osg::Vec3 eyepoint = nv->getEyePoint();

         float bd = mTerrain->GetBuildDistance(),
               bd2 = bd*2,
               x = eyepoint[0] - bd,
               y = eyepoint[1] - bd;

         for(float i=0.0f;i<=bd2;i+=mTerrain->mSegmentSize)
         {
            for(float j=0.0f;j<=bd2;j+=mTerrain->mSegmentSize)
            {
               mTerrain->BuildSegment(
                  int((x + i)/mTerrain->mSegmentSize),
                  int((y + j)/mTerrain->mSegmentSize)
               );
            }
         }

         traverse(node, nv);
      }


   private:

      /**
       * The owning SurfaceObject object.
       */
      SurfaceObject* mTerrain;
};

/**
 * The ODE SurfaceObject class identifier.
 */
static int dSurfaceObjectClass = 0;

/**
 * Constructor.
 *
 * @param name the instance name
 */
SurfaceObject::SurfaceObject(dtCore::RefPtr<dtTerrain::HeightField> heightfield,
      dtCore::RefPtr<dtTerrain::HeightField> attribute_map,
      std::vector< dtCore::RefPtr<osg::Texture2D> > texture_array,
      osg::Texture2D* terrain_texture, osg::Texture2D* terrain_mask, osg::Texture2D* terrain_recolor,
      dtCore::Object* terrain_overlay, const std::string& name)
   :  Transformable(name),
      mSegmentSize(500.0f),
      mSegmentDivisions(128),
      m_VerticalScale(1.0f),
      m_HorizontalSize(10000.0f),
      mBuildDistance(100000.0f),
      mClearFlag(false),
      m_HFMaxValue(SHRT_MIN),
      m_HFMinValue(SHRT_MAX)
{
   SetName(name);

   osg::StateSet* ss = GetOSGNode()->getOrCreateStateSet();

   ss->setMode(GL_CULL_FACE, GL_TRUE);

   GetOSGNode()->setCullCallback( new SurfaceObjectCallback(this) );

   if(dSurfaceObjectClass == 0)
   {
      dGeomClass gc;

      gc.bytes = sizeof(SurfaceObject*);
      gc.collider = GetColliderFn;
      gc.aabb = GetAABB;
      gc.aabb_test = AABBTest;
      gc.dtor = 0;

      dSurfaceObjectClass = dCreateGeomClass(&gc);
   }

   dGeomID geom = dCreateGeom(dSurfaceObjectClass);

   *(SurfaceObject**)dGeomGetClassData(geom) = this;

   SetCollisionGeom( geom );

   // This is normally set to true whenever a collision shape is set.
   // But since we are creating our own geom class here, we must enable
   // it manually.
   SetCollisionDetection( true );

   
   RegisterInstance(this);

   // Default collision category = 3
   SetCollisionCategoryBits( UNSIGNED_BIT(3) );

   // copy the pointers
   m_HeightField = heightfield;
   m_AttributeMap = attribute_map;
   m_TextureArray = texture_array;
   m_TerrainTexture = terrain_texture;
   m_TerrainMask = terrain_mask;
   m_Recolor = terrain_recolor;

   // determine max and min values of the heightfield

   m_HFMaxValue = SHRT_MIN;
   m_HFMinValue = SHRT_MAX;

   unsigned int width = m_HeightField->GetNumColumns();
   unsigned int height = m_HeightField->GetNumRows();
   unsigned int i,j;
   unsigned short int value;

   for (i = 0; i < width; i++)
     for (j = 0; j < height; j++)
       {
         value = m_HeightField->GetHeight(i,j);
         if (m_HFMaxValue < value) m_HFMaxValue = value;
         if (m_HFMinValue > value) m_HFMinValue = value;
       }

   // create overlay stuff

   // create a root node for the surface
   // m_SurfaceRoot = new osg::Group;
   // m_SurfaceRoot->setMatrix(osg::Matrix::identity());

   if (terrain_overlay != 0)
     {
       m_OverlayNode = new osgSim::OverlayNode;
       m_OverlayNode->setContinuousUpdate(true);

       // m_OverlayNode->addChild(m_SurfaceRoot.get());
       m_OverlayNode->addChild(GetMatrixNode());

       //the objects to be overlaid
       m_OverlayNode->setOverlaySubgraph(terrain_overlay->GetOSGNode());

       m_OverlayNode->setOverlayBaseHeight(-0.1);
       //m_OverlayNode->setOverlayTechnique(osgSim::OverlayNode::VIEW_DEPENDENT_WITH_ORTHOGRAPHIC_OVERLAY);
       m_OverlayNode->setOverlayTextureSizeHint(2048);
     }
}

osg::Node* SurfaceObject::GetOSGNode()
  {
    if (m_OverlayNode != 0)
      return m_OverlayNode.get();
    else
      return GetMatrixNode();
  }

const osg::Node* SurfaceObject::GetOSGNode() const
{
  if (m_OverlayNode != 0)
    return m_OverlayNode.get();
  else
    return GetMatrixNode();
}

/**
 * Destructor.
 */
SurfaceObject::~SurfaceObject()
{
   DeregisterInstance(this);
}

/**
 * Regenerates the terrain surface.
 */
void SurfaceObject::Regenerate()
{
   mClearFlag = true;
}

/**
 * Sets the size of the terrain segments.
 *
 * @param segmentSize the new segment size
 */
void SurfaceObject::SetSegmentSize(float segmentSize)
{
   mSegmentSize = segmentSize;

   mClearFlag = true;
}

/**
 * Returns the size of the terrain segments.
 *
 * @return the current segment size
 */
float SurfaceObject::GetSegmentSize() const
{
   return mSegmentSize;
}

/**
 * Sets the number of divisions in each segment.
 *
 * @param segmentDivisions the new segment divisions
 */
void SurfaceObject::SetSegmentDivisions(int segmentDivisions)
{
   mSegmentDivisions = segmentDivisions;

   mClearFlag = true;
}

/**
 * Returns the number of divisions in each segment.
 *
 * @return the current segment divisions
 */
int SurfaceObject::GetSegmentDivisions() const
{
   return mSegmentDivisions;
}

/**
 * Sets the vertical scale, which affects the feature
 * amplitude.
 *
 * @param verticalScale the new vertical scale
 */
void SurfaceObject::SetVerticalScale(float verticalScale)
{
   m_VerticalScale = verticalScale;

   mClearFlag = true;
}

/**
 * Returns the vertical scale.
 *
 * @return the vertical scale
 */
float SurfaceObject::GetVerticalScale() const
{
   return m_VerticalScale;
}

/**
 * Sets the build distance: the distance from the eyepoint for
 * which terrain is guaranteed to be generated.
 *
 * @param buildDistance the new build distance
 */
void SurfaceObject::SetBuildDistance(float buildDistance)
{
   mBuildDistance = buildDistance;
}

/**
 * Returns the build distance.
 *
 * @return the build distance
 */
float SurfaceObject::GetBuildDistance() const
{
   return mBuildDistance;
}

/**
 * Determines the height of the terrain at the specified location.
 *
 * @param x the x coordinate to check
 * @param y the y coordinate to check
 * @return the height at the specified location
 */
float SurfaceObject::GetHeight(float x, float y)
{
  float trans_x = (x/m_HorizontalSize + 0.5)*m_HeightField->GetNumColumns();
  float trans_y = (y/m_HorizontalSize + 0.5)*m_HeightField->GetNumRows();

  float normalized_height = (m_HeightField->GetInterpolatedHeight(trans_x, trans_y) - m_HFMinValue)/m_HFMaxValue;
  return m_MinAlt + (m_MaxAlt - m_MinAlt)*normalized_height;
}

/**
 * Determines the texture index at the specified location.
 *
 * @param x the x coordinate to check
 * @param y the y coordinate to check
 * @return the index at the specified location
 */
unsigned int  SurfaceObject::GetTextureIndex(float x, float y)
{
  float trans_x = (x/m_HorizontalSize + 0.5)*m_AttributeMap->GetNumColumns();
  float trans_y = (y/m_HorizontalSize + 0.5)*m_AttributeMap->GetNumRows();

  return m_AttributeMap->GetHeight((unsigned int)trans_x, (unsigned int)trans_y);
}

/**
 * Retrieves the normal of the terrain at the specified location.
 *
 * @param x the x coordinate to check
 * @param y the y coordinate to check
 * @param normal the location at which to store the normal
 * instead of triangle mesh height
 */
void SurfaceObject::GetNormal(float x, float y, osg::Vec3& normal)
{
      float z = GetHeight(x, y);

      osg::Vec3 v1(0.1f, 0.0f, GetHeight(x + 0.1f, y) - z);
      osg::Vec3 v2(0.0f, 0.1f, GetHeight(x, y + 0.1f) - z );

      normal = v1 ^ v2;

      normal.normalize();

      /*   if (normal.x() != 0 ||  normal.y() != 0 || normal.z() != 1 )
   	std::cout<<"NORM "<<"("<<normal.x()<<",\t"<<normal.y()<<",\t"<<normal.z()<<")"<<std::endl;
*/
}

/**
 * Builds a single terrain segment.
 *
 * @param x the x coordinate at which to build the segment
 * @param y the y coordinate at which to build the segment
 */
void SurfaceObject::BuildSegment(int x, int y)
{
   // return if outside bounds
   if (x*mSegmentSize >= m_HorizontalSize/2 || x*mSegmentSize < -m_HorizontalSize/2 ||
       y*mSegmentSize >= m_HorizontalSize/2 || y*mSegmentSize < -m_HorizontalSize/2)
       {
       	return;
       }

   Segment coord(x, y);

   if(mBuiltSegments.count(coord) > 0)
   {
      return;
   }
   else
   {
      mBuiltSegments.insert(coord);
   }

   int width = mSegmentDivisions + 1,
       height = mSegmentDivisions + 1;

   osg::Vec2 minimum(x * mSegmentSize, y * mSegmentSize);

   osg::LOD* lod = new osg::LOD;
   osg::Geode* geode = new osg::Geode;
   osg::Geometry* geom = new osg::Geometry;

   RefPtr<osg::Vec3Array> vertices =
      new osg::Vec3Array(width*height);

   RefPtr<osg::Vec3Array> normals =
      new osg::Vec3Array(width*height);

   RefPtr<osg::Vec4Array> colors =
      new osg::Vec4Array(width*height);

   RefPtr<osg::Vec2Array> textureCoordinates_coarse =
      new osg::Vec2Array(width*height);

   RefPtr<osg::Vec2Array> textureCoordinates_detail =
      new osg::Vec2Array(width*height);

   RefPtr<osg::Vec2Array> textureCoordinates_blending =
      new osg::Vec2Array(width*height);

   int i, j;

   // first determine the 3 detail texture indices
   std::vector<unsigned int> texture_indices(3);
   std::vector<unsigned int> texture_index_histogram( m_TextureArray.size() );

   for(i=0;i<height;i++)
   {
      for(j=0;j<width;j++)
      {
        float x = minimum[0] + j * (mSegmentSize / mSegmentDivisions),
              y = minimum[1] + i * (mSegmentSize / mSegmentDivisions);

        unsigned int texture_index = GetTextureIndex(x,y);
        texture_index_histogram[texture_index]++;
      }
   }

   unsigned int act_index = 0;
   for (unsigned int k = 0; k<texture_index_histogram.size(); k++)
     {
       if (texture_index_histogram[k] > 0)
         {
           if (act_index < 3)
             {
               texture_indices[act_index] = k;
               act_index++;
             }
           else
             {
               LOG_WARNING("Texture index is discarded, texture alignment may be wrong. Please ensure that a segment contains at most 3 different textures only.");
             }
         }
     }

   // apply textures
   osg::StateSet* ss = lod->getOrCreateStateSet();
   ss->setTextureAttribute(0, m_TerrainTexture.get() );
   ss->setTextureAttribute(2, m_TextureArray[texture_indices[0]].get() );
   ss->setTextureAttribute(3, m_TextureArray[texture_indices[1]].get() );
   ss->setTextureAttribute(4, m_TextureArray[texture_indices[2]].get() );
   ss->setTextureAttribute(5, m_TerrainMask.get() );
   ss->setTextureAttribute(6, m_Recolor.get() );

   ss->setTextureMode(0, GL_TEXTURE_2D, GL_TRUE);
   ss->setTextureMode(2, GL_TEXTURE_2D, GL_TRUE);
   ss->setTextureMode(3, GL_TEXTURE_2D, GL_TRUE);
   ss->setTextureMode(4, GL_TEXTURE_2D, GL_TRUE);
   ss->setTextureMode(5, GL_TEXTURE_2D, GL_TRUE);
   ss->setTextureMode(6, GL_TEXTURE_2D, GL_TRUE);

   for(i=0;i<height;i++)
   {
      for(j=0;j<width;j++)
      {
         float x = minimum[0] + j * (mSegmentSize / mSegmentDivisions),
               y = minimum[1] + i * (mSegmentSize / mSegmentDivisions);

         float heightAtXY = GetHeight(x, y);

         (*vertices)[i*width+j].set( x, y, heightAtXY );

         osg::Vec3 normal;

         GetNormal(x, y, normal);

         (*normals)[i*width+j].set(normal[0], normal[1], normal[2]);

         (*colors)[i*width+j] = osg::Vec4(1,1,1,1);
         // (*colors)[i*width+j] = GetColorAtXY(x, y);

         (*textureCoordinates_coarse)[i*width+j].set(x/m_HorizontalSize + 0.5, y/m_HorizontalSize + 0.5);

         (*textureCoordinates_detail)[i*width+j].set(x*0.01, y*0.01);

         unsigned int texture_index = GetTextureIndex(x,y);

         if (texture_index == texture_indices[0])
           {
             (*textureCoordinates_blending)[i*width+j].set(1, 0);
           }
         else if (texture_index == texture_indices[1])
           {
             (*textureCoordinates_blending)[i*width+j].set(0, 1);
           }
         else
           {
             (*textureCoordinates_blending)[i*width+j].set(0, 0);
           }
      }
   }

   geom->setVertexArray(vertices.get());
   geom->setNormalArray(normals.get());
   geom->setColorArray(colors.get());
   geom->setTexCoordArray(0, textureCoordinates_coarse.get());
   geom->setTexCoordArray(2, textureCoordinates_detail.get());
   geom->setTexCoordArray(3, textureCoordinates_blending.get());

   RefPtr<osg::IntArray> indices =
      new osg::IntArray(mSegmentDivisions*width*2);

   for(i=0;i<mSegmentDivisions;i++)
   {
      for(j=0;j<width;j++)
      {
         (*indices)[i*width*2 + j*2] = (i+1)*width + j;

         (*indices)[i*width*2 + j*2 + 1] = i*width + j;
      }
   }

   geom->setVertexIndices(indices.get());
   geom->setNormalIndices(indices.get());
   geom->setColorIndices(indices.get());
   geom->setTexCoordIndices(0, indices.get());
   geom->setTexCoordIndices(2, indices.get());
   geom->setTexCoordIndices(3, indices.get());

   geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);
   geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);

   for(i=0;i<mSegmentDivisions;i++)
   {
      geom->addPrimitiveSet(
         new osg::DrawArrays( osg::PrimitiveSet::TRIANGLE_STRIP, i*width*2, width*2 ) );
   }

   geom->setUseDisplayList(true);
   // geom->setUseDisplayList(false);

   geode->addDrawable(geom);

   // add shader
   std::string shader_name = "MultiTex";
   dtCore::ShaderProgram* myshader = dtCore::ShaderManager::GetInstance().FindShaderPrototype(shader_name,"TerrainShaders");
   if (myshader != 0)
   {
        dtCore::ShaderManager::GetInstance().AssignShaderFromPrototype( *myshader, *lod );
   }
   else
   {
      LOG_ERROR("Unit could not load shader for terrain with name [" + shader_name + "]");
   }

   lod->setName(GetName());

   lod->addChild(geode, 0.0f, mBuildDistance);

   GetMatrixNode()->addChild(lod);
   // m_SurfaceRoot->addChild(lod);
}

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
int SurfaceObject::Collider(dGeomID o1, dGeomID o2, int flags,
                              dContactGeom* contact, int skip)
{
   SurfaceObject* it = *(SurfaceObject**)dGeomGetClassData(o1);

   int numContacts = 0,
       maxContacts = flags & 0xFFFF;

   int geomClass = dGeomGetClass(o2);

   const dReal* position = dGeomGetPosition(o2);
   const dReal* rotation = dGeomGetRotation(o2);

   osg::Matrix mat(
      rotation[0], rotation[4], rotation[8], 0.0f,
      rotation[1], rotation[5], rotation[9], 0.0f,
      rotation[2], rotation[6], rotation[10], 0.0f,
      position[0], position[1], position[2], 1.0f
   );

   if(geomClass == dBoxClass)
   {
      dVector3 lengths;

      dGeomBoxGetLengths(o2, lengths);

      lengths[0] *= 0.5f;
      lengths[1] *= 0.5f;
      lengths[2] *= 0.5f;

      osg::Vec3 corners[8] =
      {
         osg::Vec3(-lengths[0], -lengths[1], -lengths[2]),
         osg::Vec3(-lengths[0], -lengths[1], +lengths[2]),
         osg::Vec3(-lengths[0], +lengths[1], -lengths[2]),
         osg::Vec3(-lengths[0], +lengths[1], +lengths[2]),
         osg::Vec3(+lengths[0], -lengths[1], -lengths[2]),
         osg::Vec3(+lengths[0], -lengths[1], +lengths[2]),
         osg::Vec3(+lengths[0], +lengths[1], -lengths[2]),
         osg::Vec3(+lengths[0], +lengths[1], +lengths[2])
      };

      for(int i=0;i<8 && i<maxContacts;i++)
      {
         dtUtil::MatrixUtil::TransformVec3(corners[i], mat);

         osg::Vec3 point
         (
            corners[i][0],
            corners[i][1],
            it->GetHeight(
               corners[i][0],
               corners[i][1]
            )
         );

         osg::Vec3 normal;

         it->GetNormal(
            corners[i][0],
            corners[i][1],
            normal
         );

         osg::Plane plane;
         plane.set(normal, point);

         float dist = plane.distance(corners[i]);

         if(dist <= 0.0f)
         {
            contact->pos[0] = corners[i][0] - dist*normal[0];
            contact->pos[1] = corners[i][1] - dist*normal[1];
            contact->pos[2] = corners[i][2] - dist*normal[2];

            contact->normal[0] = -normal[0];
            contact->normal[1] = -normal[1];
            contact->normal[2] = -normal[2];

            contact->depth = -dist;

            contact->g1 = o1;
            contact->g2 = o2;

            numContacts++;

            contact = (dContactGeom*)(((char*)contact) + skip);
         }
      }
   }
   else if(geomClass == dSphereClass)
   {
      dReal radius = dGeomSphereGetRadius(o2);

      osg::Vec3 center(0.0f, 0.0f, 0.0f);

      dtUtil::MatrixUtil::TransformVec3(center, mat);

      if(CollideSphere(it, center, radius, contact))
      {
         contact->g1 = o1;
         contact->g2 = o2;

         numContacts++;
      }
   }

   else if(geomClass == dCCylinderClass)
   {
      dReal pRadius, pLength;
      dGeomCCylinderGetParams(o2, &pRadius, &pLength);

      float lengthOver2 = pLength * 0.5f;

      osg::Vec3 pFrom(0.0f, 0.0f, -lengthOver2);
      osg::Vec3 pTo(0.0f, 0.0f, lengthOver2);
      osg::Vec3 pCenter(0.0f, 0.0f, 0.0f);

      dtUtil::MatrixUtil::TransformVec3(pFrom, mat);
      dtUtil::MatrixUtil::TransformVec3(pTo, mat);
      dtUtil::MatrixUtil::TransformVec3(pCenter, mat);

      if(numContacts < maxContacts && CollideSphere(it, pTo, pRadius, contact))
      {
         contact->g1 = o1;
         contact->g2 = o2;

            contact = (dContactGeom*)(((char*)contact) + skip);
//         ++contact;
         numContacts++;
      }

      if(numContacts < maxContacts && CollideSphere(it, pFrom, pRadius, contact))
      {
         contact->g1 = o1;
         contact->g2 = o2;

            contact = (dContactGeom*)(((char*)contact) + skip);
//         ++contact;
         numContacts++;
      }

      if(numContacts < maxContacts && CollideSphere(it, pCenter, pRadius, contact))
      {
         contact->g1 = o1;
         contact->g2 = o2;

            contact = (dContactGeom*)(((char*)contact) + skip);
//         ++contact;
         numContacts++;
      }


   }

   return numContacts;
}


/**
* A Helper function for Collider to detect collision with terrain between two points,
* used to do collision with a Capped Cylinder.
*
* @param pFrom the start of the line segment to collide with
* @param pTo the end of the line segment to collide with
* @param pRadius the radius of the line segment
* @param the ode contact point to fill
*/

bool SurfaceObject::CollideSphere(SurfaceObject* it, const osg::Vec3& center, float pRadius, dContactGeom* contact)
{
   osg::Vec3 point
      (
      center[0],
      center[1],
      it->GetHeight(
      center[0],
      center[1]
      )
      );

   osg::Vec3 normal;

   it->GetNormal(
      center[0],
      center[1],
      normal
      );

   osg::Plane plane;
   plane.set(normal, point);

   float dist = plane.distance(center);

   if(dist <= pRadius)
   {
      contact->pos[0] = center[0] - dist*normal[0];
      contact->pos[1] = center[1] - dist*normal[1];
      contact->pos[2] = center[2] - dist*normal[2];

      contact->normal[0] = -normal[0];
      contact->normal[1] = -normal[1];
      contact->normal[2] = -normal[2];

      contact->depth = pRadius - dist;

      return true;
   }

   return false;
}


/**
 * ODE collision function: Finds the collider function appropriate
 * to detect collisions between SurfaceObject geoms and other
 * geoms.
 *
 * @param num the class number of the geom class to check
 * @return the appropriate collider function, or NULL for none
 */
dColliderFn* SurfaceObject::GetColliderFn(int num)
{
   if(num == dBoxClass || num == dSphereClass || num == dCCylinderClass)
   {
      return Collider;
   }
   else
   {
      return NULL;
   }
}

/**
 * ODE collision function: Computes the axis-aligned bounding box
 * for SurfaceObject instances.
 *
 * @param g the geom to check
 * @param aabb the location in which to store the axis-aligned
 * bounding box
 */
void SurfaceObject::GetAABB(dGeomID g, dReal aabb[6])
{
   SurfaceObject* it = *(SurfaceObject**)dGeomGetClassData(g);

   dInfiniteAABB(g, aabb);

   // aabb[5] = it->GetVerticalScale();

   aabb[5] = HT_MAX(abs(it->m_HFMaxValue),abs(it->m_HFMinValue))*it->GetVerticalScale();
}

/**
 * ODE collision function: Checks whether the specified axis-aligned
 * bounding box intersects with an SurfaceObject instance.
 *
 * @param o1 the first (SurfaceObject) geom
 * @param o2 the second geom
 * @param aabb2 the axis-aligned bounding box of the second geom
 * @return 1 if it intersects, 0 if it does not
 */
int SurfaceObject::AABBTest(dGeomID o1, dGeomID o2, dReal aabb2[6])
{
   SurfaceObject* it = *(SurfaceObject**)dGeomGetClassData(o1);

   if(it->GetHeight(aabb2[0], aabb2[1]) >= aabb2[2] ||
      it->GetHeight(aabb2[0], aabb2[4]) >= aabb2[2] ||
      it->GetHeight(aabb2[3], aabb2[1]) >= aabb2[2] ||
      it->GetHeight(aabb2[3], aabb2[4]) >= aabb2[2])
   {
      return 1;
   }
   else
   {
      return 0;
   }
}

void SurfaceObject::SetHorizontalSize(float horizontalSize)
{
   m_HorizontalSize = horizontalSize;

   mClearFlag = true;
}

float SurfaceObject::GetHorizontalSize() const
{
   return m_HorizontalSize;
}

osg::BoundingBox SurfaceObject::GetBB()
{
	osg::BoundingBox terrainBB(-m_HorizontalSize/2, -m_HorizontalSize/2,
				m_VerticalScale*m_HFMinValue, m_HorizontalSize/2, m_HorizontalSize/2, m_VerticalScale*m_HFMaxValue);

	return terrainBB;
}
