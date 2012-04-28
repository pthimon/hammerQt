#include <prefix/dtcoreprefix-src.h>

#include <cassert>

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wextra"

#include <osg/Vec3>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Projection>
#include <osgText/Text>
#include <osg/BlendFunc>

#include <dtUtil/mathdefines.h>

#include <dtCore/deltawin.h>
#include <dtCore/transform.h>
#include <dtCore/pointaxis.h>
#include <dtCore/camera.h>

#include "camtriangle.h"

#pragma GCC diagnostic warning "-Wextra"

#include "../HTapp.h"

using namespace dtCore;
IMPLEMENT_MANAGEMENT_LAYER(CamTriangle)

// callback functor objects
class _updateCamTriangleCallback  :  public   osg::NodeCallback
{
   public:
                     _updateCamTriangleCallback( CamTriangle* camtriangle )
                     :  mCamTriangle(0)
                     {
                        assert( camtriangle );
                        mCamTriangle = camtriangle;
                     }

      virtual  void  operator()( osg::Node* node, osg::NodeVisitor* nv )
      {

         assert( node );
         assert( nv );

         osg::MatrixTransform*   xformNode   = static_cast<osg::MatrixTransform*>(node);

         Transform   xform;
         mCamTriangle.get()->GetTransform( xform );

         osg::Matrix mat;
         xform.Get( mat );

         if( dtCore::Camera*   cam   = mCamTriangle->GetCamera() )
         {
            Transform   cam_xform;
            cam->GetTransform( cam_xform );

            osg::Matrix   cam_mat;
            cam_xform.GetRotation( cam_mat );

            osg::Vec3   cam_pos;
            cam_xform.GetTranslation( cam_pos );

			float x,y;
			mCamTriangle->GetScreenPosition(x,y);

			int   wx(0L);
   			int   wy(0L);
   			int   ww(0L);
   			int   wh(0L);
			cam->GetWindow()->GetPosition( wx, wy, ww, wh );

			// calculate camera position on the minimap
			dtCore::Transform cam_transform;
    		cam->GetTransform(cam_transform);

    		osg::Vec3 cam_translation;
    		osg::Vec3 cam_rotation;
			cam_transform.GetTranslation(cam_translation);
			cam_transform.GetRotation(cam_rotation);

			osg::Vec2 cam_world_pos(cam_translation.x(), cam_translation.y());
			osg::Vec2 cam_map_pos = theApp->CalcMapPosFromWorldPos(cam_world_pos);
			float cam_screen_pos_x = ww - mCamTriangle->m_MMapX + (cam_map_pos.x()-1) * mCamTriangle->m_MMapWidth;
			float cam_screen_pos_y = cam_map_pos.y() * mCamTriangle->m_MMapHeight + mCamTriangle->m_MMapY;

            osg::Matrix comp_mat = osg::Matrix::identity();
			comp_mat *= osg::Matrix::rotate( M_PI*cam_rotation.x()/180, 0, 0, 1 );
            comp_mat *= osg::Matrix::translate( osg::Vec3( cam_screen_pos_x, cam_screen_pos_y, -100.0f ) );

            osg::Projection*  proj  = static_cast<osg::Projection*>(xformNode->getChild( 0L ));
            assert( proj );
   			proj->setMatrix( osg::Matrix::ortho2D(0.f, ww, 0.f, wh) );

            osg::MatrixTransform*   abs_xform  = static_cast<osg::MatrixTransform*>(proj->getChild( 0L ));
            assert( abs_xform );

            abs_xform->setMatrix( comp_mat );
         }

         xformNode->setMatrix( mat );

         traverse( node, nv );
      }

   private:
      RefPtr<CamTriangle> mCamTriangle;
};

// static member variables
const float                CamTriangle::DEF_SCREEN_X(100.0f);
const float                CamTriangle::DEF_SCREEN_Y(100.0f);
const float                CamTriangle::DEF_SCREEN_W(640.0f);
const float                CamTriangle::DEF_SCREEN_H(480.0f);

// default constructor
CamTriangle::CamTriangle( Camera* cam )
:
   mCamera(cam),
   mScreenX(DEF_SCREEN_X),
   mScreenY(DEF_SCREEN_Y),
   mScreenW(DEF_SCREEN_W),
   mScreenH(DEF_SCREEN_H)
{
   RegisterInstance( this );

   ctor();

   //hookup an update callback on this node
   GetOSGNode()->setUpdateCallback( new _updateCamTriangleCallback( this ) );

   // Default collision category = 2
   SetCollisionCategoryBits( UNSIGNED_BIT(2) );
}



// destructor
CamTriangle::~CamTriangle()
{
   DeregisterInstance( this );
}



/** PUBLIC MEMBER FUNCTIONS */

void
CamTriangle::GetScreenPosition( float& x, float& y )   const
{
   x  = mScreenX;
   y  = mScreenY;
}

osg::Vec2
CamTriangle::GetScreenPosition()   const
{
   return osg::Vec2( mScreenX, mScreenY );
}

void
CamTriangle::SetScreenPosition( float x, float y )
{
   mScreenX = x;
   mScreenY = y;
}


void
CamTriangle::SetScreenPosition( const osg::Vec2& xy )
{
   SetScreenPosition( xy.x(), xy.y() );
}

dtCore::Camera*
CamTriangle::GetCamera()
{
   return   mCamera.get();
}



void
CamTriangle::SetCamera( dtCore::Camera* cam )
{
   mCamera  = cam;

   SetWindow( cam->GetWindow() );
}



/** PRIVATE MEMBER FUNCTIONS */
void
CamTriangle::ctor()
{
    // create the Geode (Geometry Node) to contain all our osg::Geometry objects.
    osg::Geode* geode = new osg::Geode();

    // create Geometry object to store all the vetices and lines primtive.
    osg::Geometry* polyGeom = new osg::Geometry();

    // set the colors as before, use a ref_ptr rather than just
    // standard C pointer, as that in the case of it not being
    // assigned it will still be cleaned up automatically.
    osg::ref_ptr<osg::Vec4Array> my_colors = new osg::Vec4Array;
    my_colors->push_back(osg::Vec4(1.0f,1.0f,0.0f,1.0f));
    my_colors->push_back(osg::Vec4(1.0f,1.0f,0.0f,0.0f));
    my_colors->push_back(osg::Vec4(1.0f,1.0f,0.0f,0.0f));

    // same trick for shared normal.
    osg::ref_ptr<osg::Vec3Array> shared_normals = new osg::Vec3Array;
    shared_normals->push_back(osg::Vec3(0.0f,-1.0f,0.0f));

    osg::Vec3 myCoords[] =
    {
        osg::Vec3(0, 0, 0),
        osg::Vec3(-10, 20, 0),
        osg::Vec3(10, 20, 0)
	};

    int numCoords = sizeof(myCoords)/sizeof(osg::Vec3);

    osg::Vec3Array* vertices = new osg::Vec3Array(numCoords,myCoords);

    // pass the created vertex array to the points geometry object.
    polyGeom->setVertexArray(vertices);

    // use the shared color array.
    polyGeom->setColorArray(my_colors.get());
    polyGeom->setColorBinding(osg::Geometry::BIND_PER_VERTEX);

    // use the shared normal array.
    polyGeom->setNormalArray(shared_normals.get());
    polyGeom->setNormalBinding(osg::Geometry::BIND_OVERALL);

    // This time we simply use primitive, and hardwire the number of coords to use
    // since we know up front,
    polyGeom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::TRIANGLES,0,3));

    osg::StateSet* stateSet = geode->getOrCreateStateSet();

    stateSet->setRenderBinDetails( 20L, "RenderBin" );
    stateSet->setMode( GL_LIGHTING, osg::StateAttribute::OFF );
    stateSet->setMode( GL_BLEND, osg::StateAttribute::ON );
    stateSet->setMode( GL_DEPTH_TEST, osg::StateAttribute::OFF );

    osg::BlendFunc *fn = new osg::BlendFunc();
    fn->setFunction(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::DST_ALPHA);
    stateSet->setAttributeAndModes(fn, osg::StateAttribute::ON);

    // add the points geomtry to the geode.
    geode->addDrawable(polyGeom);

   osg::MatrixTransform*   modelview_abs  = new osg::MatrixTransform;
   assert( modelview_abs );

   modelview_abs->setReferenceFrame( osg::Transform::ABSOLUTE_RF );
   modelview_abs->setMatrix( osg::Matrix::identity() );
   modelview_abs->addChild( geode );

   osg::Projection*  projection  = new osg::Projection;
   assert( projection );

   projection->setMatrix( osg::Matrix::ortho2D(0.f, mScreenW, 0.f, mScreenH) );
   projection->addChild( modelview_abs );


   GetMatrixNode()->addChild( projection );


   SetName( "CamTriangle" );

   if( mCamera.get() )
   {
      SetWindow( mCamera->GetWindow() );
   }
}



void
CamTriangle::SetWindow( dtCore::DeltaWin* win )
{
   if( win == 0 )
   {
      return;
   }

   int   x(0L);
   int   y(0L);
   int   w(0L);
   int   h(0L);

   win->GetPosition( x, y, w, h );
   assert( w > 0 );
   assert( h > 0 );

   mScreenW = float(w);
   mScreenH = float(h);

   mScreenX = mScreenW * 0.5f;
   mScreenY = mScreenH * 0.5f;
}
