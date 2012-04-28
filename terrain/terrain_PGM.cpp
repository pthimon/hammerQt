//////////////////////////////////////////////////////////////////////
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wextra"

#include <prefix/dtcoreprefix-src.h>
#include <dtCore/globals.h>
#include <dtCore/light.h>
#include <dtUtil/mathdefines.h>
#include <dtUtil/matrixutil.h>
#include <dtUtil/log.h>
#include <dtTerrain/heightfield.h>
#include <vector>
#include <cmath>

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

#pragma GCC diagnostic warning "-Wextra"

#include "terrain_PGM.h"
#include "../HTapp.h"

using namespace dtCore;

IMPLEMENT_MANAGEMENT_LAYER(PGMTerrain)

/**
 * The terrain callback class.  Builds terrain segments
 * around viewer.
 */
class PGMTerrainCallback : public osg::NodeCallback
{
   public:

      /**
       * Constructor.
       *
       * @param terrain the owning PGMTerrain object
       */
      PGMTerrainCallback(PGMTerrain* terrain)
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
       * The owning PGMTerrain object.
       */
      PGMTerrain* mTerrain;
};

/**
 * The ODE PGMTerrain class identifier.
 */
static int dPGMTerrainClass = 0;

/**
 * Constructor.
 *
 * @param name the instance name
 */
PGMTerrain::PGMTerrain(const std::string& name, osg::Image* textureImage)
   :  Transformable(name),
      mSegmentSize(500.0f),
      mSegmentDivisions(128),
      mHorizontalScale(0.0035f),
      mVerticalScale(30.0f),
      mBuildDistance(3000.0f),
      mSmoothCollisionsEnabled(true),
      mClearFlag(false),
      mLOSPostSpacing(0.f),
   	  m_foldingBoundary(0.0f),
   	  m_HorizontalSize(10000.0f),
   	  m_HFMaxValue(SHRT_MIN),
   	  m_HFMinValue(SHRT_MAX)
{
   SetName(name);

   osg::StateSet* ss = GetOSGNode()->getOrCreateStateSet();

   ss->setMode(GL_CULL_FACE, GL_TRUE);

   osg::Image* image = 0;

   if (textureImage != 0)
   {
      image = textureImage;
   }
   else
   {
      image = new osg::Image;

      dtUtil::Noise2f texNoise;

      unsigned char* texture = new unsigned char[256*256*3];

      int k = 0;

      for(int i=0;i<256;i++)
      {
         for(int j=0;j<256;j++)
         {
            float val = 0.7f + texNoise.GetNoise(osg::Vec2f(i*0.1f, j*0.1f))*0.3f;

            texture[k++] = dtUtil::Min( 50 + (unsigned char)(val*255), 255);
            texture[k++] = dtUtil::Min( 50 + (unsigned char)(val*255), 255);
            texture[k++] = dtUtil::Min( 50 + (unsigned char)(val*255), 255);
         }
      }

      image->setImage(
         256, 256, 1, 3, GL_RGB, GL_UNSIGNED_BYTE,
         texture, osg::Image::USE_NEW_DELETE
         );
   }

   osg::Texture2D* tex = new osg::Texture2D(image);

   tex->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
   tex->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);

   osg::TexEnv* texenv = new osg::TexEnv;
   texenv->setMode(osg::TexEnv::MODULATE);

   ss->setTextureAttribute(0, tex);
   ss->setTextureAttribute(0, texenv);

   ss->setTextureMode(0, GL_TEXTURE_2D, GL_TRUE);
   // ss->setTextureMode(0, GL_TEXTURE_2D, GL_FALSE);

   /*
   osg::ShadeModel* shademodel = new osg::ShadeModel;
   ss->setAttribute(shademodel);
   shademodel->setMode( osg::ShadeModel::SMOOTH );
   */

   GetOSGNode()->setCullCallback( new PGMTerrainCallback(this) );

   if(dPGMTerrainClass == 0)
   {
      dGeomClass gc;

      gc.bytes = sizeof(PGMTerrain*);
      gc.collider = GetColliderFn;
      gc.aabb = GetAABB;
      gc.aabb_test = AABBTest;
      gc.dtor = 0;

      dPGMTerrainClass = dCreateGeomClass(&gc);
   }

   dGeomID geom = dCreateGeom(dPGMTerrainClass);

   *(PGMTerrain**)dGeomGetClassData(geom) = this;

   SetCollisionGeom( geom );

   // This is normally set to true whenever a collision shape is set.
   // But since we are creating our own geom class here, we must enable
   // it manually.
   SetCollisionDetection( true );

   RegisterInstance(this);

   // Default collision category = 3
   SetCollisionCategoryBits( UNSIGNED_BIT(3) );

   SetLineOfSightSpacing(25.f); // a bit less than DTED L2

   // allocate heightfield class
   m_HeightField = new dtTerrain::HeightField();
}

/**
 * Destructor.
 */
PGMTerrain::~PGMTerrain()
{
   DeregisterInstance(this);
}

/**
 * Regenerates the terrain surface.
 */
void PGMTerrain::Regenerate()
{
   mClearFlag = true;
}

/**
 * Sets the size of the terrain segments.
 *
 * @param segmentSize the new segment size
 */
void PGMTerrain::SetSegmentSize(float segmentSize)
{
   mSegmentSize = segmentSize;

   mClearFlag = true;
}

/**
 * Returns the size of the terrain segments.
 *
 * @return the current segment size
 */
float PGMTerrain::GetSegmentSize() const
{
   return mSegmentSize;
}

/**
 * Sets the number of divisions in each segment.
 *
 * @param segmentDivisions the new segment divisions
 */
void PGMTerrain::SetSegmentDivisions(int segmentDivisions)
{
   mSegmentDivisions = segmentDivisions;

   mClearFlag = true;
}

/**
 * Returns the number of divisions in each segment.
 *
 * @return the current segment divisions
 */
int PGMTerrain::GetSegmentDivisions() const
{
   return mSegmentDivisions;
}

/**
 * Sets the horizontal scale, which affects the
 * feature frequency.
 *
 * @param horizontalScale the new horizontal scale
 */
void PGMTerrain::SetHorizontalScale(float horizontalScale)
{
   mHorizontalScale = horizontalScale;

   mClearFlag = true;
}

/**
 * Returns the horizontal scale.
 *
 * @return the horizontal scale
 */
float PGMTerrain::GetHorizontalScale() const
{
   return mHorizontalScale;
}

/**
 * Sets the vertical scale, which affects the feature
 * amplitude.
 *
 * @param verticalScale the new vertical scale
 */
void PGMTerrain::SetVerticalScale(float verticalScale)
{
   mVerticalScale = verticalScale;

   mClearFlag = true;
}

/**
 * Returns the vertical scale.
 *
 * @return the vertical scale
 */
float PGMTerrain::GetVerticalScale() const
{
   return mVerticalScale;
}

/**
 * Sets the build distance: the distance from the eyepoint for
 * which terrain is guaranteed to be generated.
 *
 * @param buildDistance the new build distance
 */
void PGMTerrain::SetBuildDistance(float buildDistance)
{
   mBuildDistance = buildDistance;
}

/**
 * Returns the build distance.
 *
 * @return the build distance
 */
float PGMTerrain::GetBuildDistance() const
{
   return mBuildDistance;
}

/**
 * Enables or disables smooth collision detection (collision detection
 * based on the underlying noise function, rather than the triangle
 * mesh).
 *
 * @param enable true to enable, false to disable
 */
void PGMTerrain::EnableSmoothCollisions(bool enable)
{
   mSmoothCollisionsEnabled = enable;
}

/**
 * Checks whether smooth collision detection is enabled.
 *
 * @return true if enabled, false if disabled
 */
bool PGMTerrain::SmoothCollisionsEnabled() const
{
   return mSmoothCollisionsEnabled;
}

//initializes info used for the GetColor function
void PGMTerrain::SetupColorInfo()
{
   //this is set based on the get height function
   //since the GetNoise will return a value from 0-1
   //the min and max height could be as follows

   // mMinHeight = -2.0f * mVerticalScale;
   // mMaxHeight = 2.0f * mVerticalScale;

   mMinHeight = mVerticalScale * (float)m_HFMinValue;
   mMaxHeight = mVerticalScale * (float)m_HFMaxValue;

   float difference = mMaxHeight - mMinHeight;
   float increment = difference / 3.0f;

   mMinColorIncrement = (increment * 2.0f);
   mMaxColorIncrement = increment;

   mIdealHeight = mMinColorIncrement + mMinHeight;

   //set the colors to interpolate between
   mMaxColor.set((255.0f / 255.0f), (200.0f / 255.0f), (200.0f / 255.0f));
   mMinColor.set((240.0f / 255.0f), (255.0f / 255.0f), (100.0f / 255.0f));
   mIdealColor.set((255.0f / 255.0f), (255.0f / 255.0f), (255.0f / 255.0f));

}

//returns an interpolated color based on the height
osg::Vec4 PGMTerrain::GetColor(float height)
{
   float r,g,b;

   float minPercent, maxPercent;
   osg::Vec3* minColor;
   osg::Vec3* maxColor;

   minPercent = std::min<float>(std::max<float>(0.0f, (mIdealHeight - height) / mMinColorIncrement), 1.0f);
   maxPercent = 1 - minPercent;
   maxColor = &mIdealColor;
   minColor = &mMinColor;

   r = (*maxColor)[0] * maxPercent;
   g = (*maxColor)[1] * maxPercent;
   b = (*maxColor)[2] * maxPercent;


   r += (*minColor)[0] * minPercent;
   g += (*minColor)[1] * minPercent;
   b += (*minColor)[2] * minPercent;

   // g += (*minColor)[1] * minPercent * minPercent;
   // b += (*minColor)[2] * minPercent * minPercent;

   return osg::Vec4(r, g, b, 1.0f);
}

osg::Vec4 PGMTerrain::GetColorAtXY(float x, float y)
{
   float height = GetHeight(x,y,true);
   osg::Vec3 normal;
   GetNormal(x, y, normal, true);
   osg::Vec4 light_dir4 = theApp->GetScene()->GetLight("MyLight")->GetLightSource()->getLight()->getPosition();
   osg::Vec3 light_dir(light_dir4.x(), light_dir4.y(), light_dir4.z());

   // now we have all the data...

   // calculate angle between light and normal
   // float angle = (normal * light_dir)/2+0.5;
   float angle = std::acos(normal * light_dir)/M_PI;

/*   osg::Vec4 result = GetColor(height);
   result.x() *= angle;
   result.y() *= angle;
   result.z() *= angle;

   return result;*/

   double ratio;
   osg::Vec3 color;
   if (height > mIdealHeight)
   {
   	 ratio = (height-mIdealHeight)/(mMaxHeight-mIdealHeight);
   	 color = (mMaxColor*ratio + mIdealColor*(1-ratio) )*angle;
   }
   else
   {
	 ratio = (height-mMinHeight)/(mIdealHeight-mMinHeight);
   	 color = (mIdealColor*ratio + mMinColor*(1-ratio) )*angle;
   }

   return osg::Vec4(color.x(), color.y(), color.z(), 1.0f);
}


/**
 * Determines the height of the terrain at the specified location.
 *
 * @param x the x coordinate to check
 * @param y the y coordinate to check
 * @param smooth if true, use height of underlying noise function
 * instead of triangle mesh height
 * @return the height at the specified location
 */
float PGMTerrain::GetHeight(float x, float y, bool smooth)
{
   if(smooth)
   {
   	float trans_x = (x/m_HorizontalSize + 0.5)*m_HeightField->GetNumColumns();
	float trans_y = (y/m_HorizontalSize + 0.5)*m_HeightField->GetNumRows();

	// float index_x = round( (x/m_HorizontalSize + 0.5)*m_HeightField->GetNumColumns() );
	// float index_y = round( (y/m_HorizontalSize + 0.5)*m_HeightField->GetNumRows() );
    // return mVerticalScale * m_HeightField->GetHeight((int)index_x, (int)index_y);

    return mVerticalScale * m_HeightField->GetInterpolatedHeight(trans_x, trans_y);

//      osg::Vec2f osgvec((x + mBuildDistance) * mHorizontalScale, (y + mBuildDistance) * mHorizontalScale);
//      return mVerticalScale * 2.0f * mNoise.GetNoise(osgvec) - 1.0f;
   }
   else
   {
      float scale = mSegmentSize / mSegmentDivisions;

      x /= scale;
      y /= scale;

      float fx = floor(x), fy = floor(y),
            cx = ceil(x), cy = ceil(y),
            ix = x - fx, iy = y - fy;

      if(ix < iy)
      {
         float p00 = GetHeight(fx*scale, fy*scale, true),
               p01 = GetHeight(fx*scale, cy*scale, true),
               p11 = GetHeight(cx*scale, cy*scale, true),
               p00_01 = p00 + iy*(p01-p00);

         return p00_01 + ix*(p11 - p00_01);
      }
      else
      {
         float p00 = GetHeight(fx*scale, fy*scale, true),
               p10 = GetHeight(cx*scale, fy*scale, true),
               p11 = GetHeight(cx*scale, cy*scale, true),
               p10_11 = p10 + iy*(p11-p10);

         return p00 + ix*(p10_11 - p00);
      }
   }
}

/**
 * Retrieves the normal of the terrain at the specified location.
 *
 * @param x the x coordinate to check
 * @param y the y coordinate to check
 * @param normal the location at which to store the normal
 * @param smooth if true, use height of underlying noise function
 * instead of triangle mesh height
 */
void PGMTerrain::GetNormal(float x, float y, osg::Vec3& normal, bool smooth)
{
   if(smooth)
   {
      float z = GetHeight(x, y, true);

      osg::Vec3 v1(0.1f, 0.0f, GetHeight(x + 0.1f, y, true) - z);
      osg::Vec3 v2(0.0f, 0.1f, GetHeight(x, y + 0.1f, true) - z );

      normal = v1 ^ v2;

      normal.normalize();
   }
   else
   {
      float scale = mSegmentSize / mSegmentDivisions;

      x /= scale;
      y /= scale;

      float fx = floor(x), fy = floor(y),
            cx = ceil(x), cy = ceil(y),
            ix = x - fx, iy = y - fy;

      if(ix < iy)
      {
         float p00 = GetHeight(fx*scale, fy*scale, true),
               p01 = GetHeight(fx*scale, cy*scale, true),
               p11 = GetHeight(cx*scale, cy*scale, true);

         osg::Vec3 v1(0.0f, -scale, p00 - p01);
         osg::Vec3 v2(scale, 0.0f, p11 - p01);

         normal = v1 ^ v2;

         normal.normalize();
      }
      else
      {
         float p00 = GetHeight(fx*scale, fy*scale, true),
               p10 = GetHeight(cx*scale, fy*scale, true),
               p11 = GetHeight(cx*scale, cy*scale, true);

         osg::Vec3 v1(0.0f, scale, p11 - p10);
         osg::Vec3 v2(-scale, 0.0f, p00 - p10 );

         normal = v1 ^ v2;

         normal.normalize();
      }
   }

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
void PGMTerrain::BuildSegment(int x, int y)
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

   osg::LOD* lod = new osg::LOD;

   osg::Geode* geode = new osg::Geode;

   osg::Geometry* geom = new osg::Geometry;

   int width = mSegmentDivisions + 1,
       height = mSegmentDivisions + 1;

   osg::Vec2 minimum(x * mSegmentSize, y * mSegmentSize);

   RefPtr<osg::Vec3Array> vertices =
      new osg::Vec3Array(width*height);

   RefPtr<osg::Vec3Array> normals =
      new osg::Vec3Array(width*height);

   RefPtr<osg::Vec4Array> colors =
      new osg::Vec4Array(width*height);

   RefPtr<osg::Vec2Array> textureCoordinates =
      new osg::Vec2Array(width*height);

   int i, j;

   for(i=0;i<height;i++)
   {
      for(j=0;j<width;j++)
      {
         float x = minimum[0] + j * (mSegmentSize / mSegmentDivisions),
               y = minimum[1] + i * (mSegmentSize / mSegmentDivisions);

         float heightAtXY = GetHeight(x, y, true);

         (*vertices)[i*width+j].set(
            x, y,
            heightAtXY
         );

         osg::Vec3 normal;

         GetNormal(x, y, normal, true);

         (*normals)[i*width+j].set(normal[0], normal[1], normal[2]);

         (*colors)[i*width+j] = GetColorAtXY(x, y);

         (*textureCoordinates)[i*width+j].set(x*0.1, y*0.1);
      }
   }

   geom->setVertexArray(vertices.get());

   geom->setNormalArray(normals.get());

   geom->setColorArray(colors.get());

   geom->setTexCoordArray(0, textureCoordinates.get());

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

   geom->setNormalBinding(osg::Geometry::BIND_PER_VERTEX);

   geom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);

   for(i=0;i<mSegmentDivisions;i++)
   {
      geom->addPrimitiveSet(
         new osg::DrawArrays(
            osg::PrimitiveSet::TRIANGLE_STRIP,
            i*width*2,
            width*2
         )
      );
   }

   geom->setUseDisplayList(true);

   geode->addDrawable(geom);

   // geode->setName(GetName());
   // geom->setName(GetName());
   lod->setName(GetName());

   lod->addChild(geode, 0.0f, mBuildDistance);

   GetMatrixNode()->addChild(lod);
}

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
int PGMTerrain::Collider(dGeomID o1, dGeomID o2, int flags,
                              dContactGeom* contact, int skip)
{
   PGMTerrain* it = *(PGMTerrain**)dGeomGetClassData(o1);

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
               corners[i][1],
               it->mSmoothCollisionsEnabled
            )
         );

         osg::Vec3 normal;

         it->GetNormal(
            corners[i][0],
            corners[i][1],
            normal,
            true
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

bool PGMTerrain::CollideSphere(PGMTerrain* it, const osg::Vec3& center, float pRadius, dContactGeom* contact)
{
   osg::Vec3 point
      (
      center[0],
      center[1],
      it->GetHeight(
      center[0],
      center[1],
      it->mSmoothCollisionsEnabled
      )
      );

   osg::Vec3 normal;

   it->GetNormal(
      center[0],
      center[1],
      normal,
      true
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
 * to detect collisions between PGMTerrain geoms and other
 * geoms.
 *
 * @param num the class number of the geom class to check
 * @return the appropriate collider function, or NULL for none
 */
dColliderFn* PGMTerrain::GetColliderFn(int num)
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
 * for PGMTerrain instances.
 *
 * @param g the geom to check
 * @param aabb the location in which to store the axis-aligned
 * bounding box
 */
void PGMTerrain::GetAABB(dGeomID g, dReal aabb[6])
{
   PGMTerrain* it = *(PGMTerrain**)dGeomGetClassData(g);

   dInfiniteAABB(g, aabb);

   // aabb[5] = it->GetVerticalScale();

   aabb[5] = HT_MAX(abs(it->m_HFMaxValue),abs(it->m_HFMinValue))*it->GetVerticalScale();
}

/**
 * ODE collision function: Checks whether the specified axis-aligned
 * bounding box intersects with an PGMTerrain instance.
 *
 * @param o1 the first (PGMTerrain) geom
 * @param o2 the second geom
 * @param aabb2 the axis-aligned bounding box of the second geom
 * @return 1 if it intersects, 0 if it does not
 */
int PGMTerrain::AABBTest(dGeomID o1, dGeomID o2, dReal aabb2[6])
{
   PGMTerrain* it = *(PGMTerrain**)dGeomGetClassData(o1);

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

bool PGMTerrain::IsClearLineOfSight( const osg::Vec3& pointOne,
                                         const osg::Vec3& pointTwo )
{
   // A smarter version would check basic generation parameters
   // to see if two points are above any generated terrain

   // cant see undergound (or underwater)
   //if (pointOne.z() < 0 || pointTwo.z() < 0)
   //   return false;

   osg::Vec3 ray = pointTwo - pointOne;
   double length( ray.length() );
   // If closer than post spacing, then clear LOS
   if( length < GetLineOfSightSpacing() )
   {
      return true;
   }

   float stepsize( GetLineOfSightSpacing() / length );
   double s( 0.0 );

   while( s < 1.0 )
   {
      osg::Vec3 testPt = pointOne + ray*s;
      double h( GetHeight( testPt.x(), testPt.y() ) );

      // Segment blocked by terrain
      if( h >= testPt.z() )
      {
         return false;
      }
      s += stepsize;
   }

   // Walked full ray, so clear LOS
   return true;
}

bool PGMTerrain::Load(const std::string & filename)
{
  std::ostringstream msg;

  std::string pgmPath = dtCore::FindFileInPathList( filename );

  if (pgmPath.empty())
  {
     LOG_WARNING("Could not find PGM file to load.  Missing file name is """ + filename + """." );
     return false;
  }

  // open the .pgm for reading
  std::ifstream pgm_stream(pgmPath.c_str(), std::ios::in);

  if (!pgm_stream.good())
  {
	msg.clear();
    msg << "Error while loading PGM file:" << pgmPath << std::endl;
	dtUtil::Log::GetInstance().LogMessage(__FUNCTION__, __LINE__, msg.str(), dtUtil::Log::LOG_ERROR);
  	return false;
  }

  // skip trailing code
  char char_buf[80];
  pgm_stream.get(char_buf, 80);

  int xsize, ysize, maxvalues;

  pgm_stream >> xsize;
  pgm_stream >> ysize;
  pgm_stream >> maxvalues;

  msg.clear();
  msg << "PGM size is:" << xsize << "x" << ysize << ", maximum value is:" << maxvalues << std::endl;
  dtUtil::Log::GetInstance().LogMessage(__FUNCTION__, __LINE__, msg.str(), dtUtil::Log::LOG_DEBUG);

  // check size
  if (xsize != ysize)
  {
  	LOG_ERROR("PGM x and y dimension mismatch.");
  	return false;
  }

  // allocate the new heightfield
  m_HeightField->Allocate(xsize, ysize);

  double foldingSize = m_foldingBoundary * xsize;
  double x_folding_ratio, y_folding_ratio;

  m_HFMaxValue = SHRT_MIN;
  m_HFMinValue = SHRT_MAX;

  unsigned short value;
  double new_value;
  signed short hf_value;

  for (int i=0; i<xsize; i++)
  	for (int j=0; j<ysize; j++)
        {
        	pgm_stream >> value;

            if (!pgm_stream.good())
            {
            	LOG_ERROR("Error while reading PGM file.");
			}

        	x_folding_ratio = 1;
        	y_folding_ratio = 1;

			if (i < foldingSize) x_folding_ratio = i/foldingSize;
			else if (i > xsize-foldingSize) x_folding_ratio = (xsize-i-1)/foldingSize;

			if (j < foldingSize) y_folding_ratio = j/foldingSize;
			else if (j > xsize-foldingSize) y_folding_ratio = (ysize-j-1)/foldingSize;

        	// project into the [SHRT_MIN..SHRT_MAX] range with folding
			new_value = floor((float)value*x_folding_ratio*y_folding_ratio);
        	hf_value = (short)(new_value + SHRT_MIN);

        	// flip up-down and rotate
        	m_HeightField->SetHeight(j, xsize-i-1, hf_value);

        	// update max-min values of heightfield
			if (hf_value > m_HFMaxValue) m_HFMaxValue = hf_value;
			if (hf_value < m_HFMinValue) m_HFMinValue = hf_value;
         }

  pgm_stream.close();

  SetName(filename);

  SetupColorInfo();

  mClearFlag = true;

  return true;
}

void PGMTerrain::SetHorizontalSize(float horizontalSize)
{
   m_HorizontalSize = horizontalSize;

   mClearFlag = true;
}

float PGMTerrain::GetHorizontalSize() const
{
   return m_HorizontalSize;
}

osg::BoundingBox PGMTerrain::GetBB()
{
	osg::BoundingBox terrainBB(-m_HorizontalSize/2, -m_HorizontalSize/2,
				mVerticalScale*m_HFMinValue, m_HorizontalSize/2, m_HorizontalSize/2, mVerticalScale*m_HFMaxValue);

	return terrainBB;
}
