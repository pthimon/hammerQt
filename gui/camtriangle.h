#ifndef CAMTRIANGLE_H_
#define CAMTRIANGLE_H_

//#include <dtCore/macros.h>
#include <dtCore/refptr.h>
#include <dtCore/transformable.h>
#include <dtCore/camera.h>
#include <dtCore/deltawin.h>

#include "../common.h"

//class dtCore::Camera;
//class dtCore::DeltaWin;

class DT_CORE_EXPORT CamTriangle :   public   dtCore::Transformable
{
	DECLARE_MANAGEMENT_LAYER(CamTriangle)
	
	friend class _updateCamTriangleCallback;

public:
	CamTriangle( dtCore::Camera* cam );
	void GetScreenPosition( float& x, float& y ) const;
	osg::Vec2 GetScreenPosition() const;
	void SetScreenPosition( float x, float y );
    void SetScreenPosition( const osg::Vec2& xy );
    /**
     * Sets the minimap area. Values from the lower right corner, inversely.
     */
    void SetMinimapArea(float x, float y, float w, float h)
    { m_MMapX = x; m_MMapY = y; m_MMapWidth = w; m_MMapHeight = h; } 

	///@return returns a pointer to the current camera
	dtCore::Camera* GetCamera();
 
 	//@param sets the current camera
	void SetCamera( dtCore::Camera* cam );

protected:
	virtual ~CamTriangle();
      
private:
     inline void ctor();
     inline void SetWindow( dtCore::DeltaWin* win );

     dtCore::RefPtr<dtCore::Camera> mCamera;    /// camera who's window we place the model
     
     float mScreenX;   /// screen position of model
 	 float mScreenY;   /// screen position of model
	 float mScreenW;   /// screen width
     float mScreenH;   /// screen height
     
     float m_MMapX;
     float m_MMapY;
     float m_MMapWidth;
     float m_MMapHeight;

	 static const float MAX_SCREEN_X;
	 static const float MAX_SCREEN_Y;
	 static const float MIN_SCREEN_X;
	 static const float MIN_SCREEN_Y;
	 static const float DEF_SCREEN_X;
	 static const float DEF_SCREEN_Y;
	 static const float DEF_SCREEN_W;
	 static const float DEF_SCREEN_H;
	 static const float DEF_AXIS_SIZE;
};

#endif // CAMTRIANGLE_H_
