#ifndef TERRAIN_H_
#define TERRAIN_H_

//////////////////////////////////////////////////////////////////////

#pragma GCC diagnostic ignored "-Wextra"

#include "../common.h"

#include <dtCore/base.h>
#include <dtCore/transformable.h>
#include <dtUtil/noiseutility.h>
#include <dtTerrain/heightfield.h>
#include <osg/Vec2>
#include <osg/Vec3>
#include <osg/Texture2D>
#include <ode/collision.h>
#include <set>
#include <list>
#include <agg2/agg_pixfmt_rgba.h>
#include <agg2/agg_renderer_base.h>
#include <agg2/agg_path_storage.h>

#pragma GCC diagnostic warning "-Wextra"

class TerrainLayer;
//class SurfaceObject;
#include "SurfaceObject.h"

namespace dtCore
{
  class Object;
}

// for convenience, we expect a vector of osg::Vec2's to be presented
typedef agg::path_base<agg::vertex_stl_storage<std::vector<agg::vertex_d> > > stl_path_storage; 

enum dash_type { NORMAL, DOT, LINE, DOT_LINE, DOT_DOT_LINE };

/** Structure storing a path. This class can hide the agg library from the user by presenting a 
 *  an std::vector<vertex> of world 2D coordinates in the "vertices" variable, which is automatically converted
 *  into an AGG path in mask space with the AddPath() command if the agg_path member is empty in this
 *  structure. If you need to fine-tune the AGG path with AGG function calls, you can use the functions of
 *  agg_path_storage.h on the "agg_path" member in the path_base class. Beware: coordinates in agg_path are mask
 * coordinates, while those in vertices are real-world coordinates!
 */
struct PathPrimitive
{
  PathPrimitive()
    {
      width = 1;
      dash = NORMAL;
      start_marker = false;
      end_marker = false;
      smooth = 1;
      closed = false;
      filled = false;
      color = agg::rgba(1,1,1);
    }
    
  std::vector<osg::Vec2> vertices;
  stl_path_storage agg_path;
  
  // these variables affect the rendering, so they have an effect when DrawPrimitives() is called. 
  float width;
  dash_type dash;
  bool start_marker;
  bool end_marker;
  bool filled;
  agg::rgba color;  
  // smoothness factor (if >0 then the path is smoothed)
  float smooth;
  
  // closed has an effect only when the path is added by the AddPath() function
  bool closed;
};

/**
* A terrain class for loading and handling layered terrains.
*/
class DT_CORE_EXPORT Terrain : public dtCore::Base
{
	DECLARE_MANAGEMENT_LAYER(Terrain)

public:

      enum layerset {
        // This set contains the surface object only.
        LAYERSET_SURFACE
      };

     /**
      * Constructor.
      *
      * @param filename The file to read the data from.
      * @param use_overlay_node If true, an overlay node is added.
      * @param node The node to assign all created nodes to.
      */
     Terrain(const std::string & filename, bool use_overlay_node);

     /** Returns the name of the .tha holding the minimap file with full path.
      */
     const std::string & GetMinimapFileName() { return m_MinimapFileName; }

     /**
      * Returns the horizontal size.
      *
      * @return the horizontal size
      */
     float GetHorizontalSize() const;

     /**
      * Determines the height of the terrain at the specified location.
      *
      * @param x the x coordinate to check
      * @param y the y coordinate to check
      * @param layer The layer name to use, default is the basic surface
      * @return the height at the specified location
      */
     float GetHeight(float x, float y,
         Terrain::layerset setname = LAYERSET_SURFACE);

     /**
      * Retrieves the normal of the terrain at the specified location.
      *
      * @param x the x coordinate to check
      * @param y the y coordinate to check
      * @param normal the location at which to store the normal
      * @param layer The layer name to use, default is the basic surface
      * instead of triangle mesh height
      */
     void GetNormal(float x, float y, osg::Vec3& normal,
         Terrain::layerset setname = LAYERSET_SURFACE);

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
     bool IsClearLineOfSight( const osg::Vec3& pointOne, const osg::Vec3& pointTwo,
         Terrain::layerset setname = LAYERSET_SURFACE);

     /**
       * Set the stepping distance to sample points for the line of sight calculation (meters).
       * Defaults to 25 meters.
       */
     void SetLineOfSightSpacing(float spacing) {m_LOSPostSpacing = spacing;}

     /** Gets the stepping distance for the LOS calculation.
      * @return The spacing value.
      */
     float GetLineOfSightSpacing() const {return m_LOSPostSpacing; }

     /** Returns the surface heightfield determined from the layers.
      * return pointer to the underlying heightfield object.
      */
     dtTerrain::HeightField* GetHeightField() { return m_HeightField.get(); }

     /** Returns the map's name.
      */
     const std::string & GetName() { return m_MapName; }

     /**
      * Returns the terrain's bounding box.
      */
     osg::BoundingBox GetBB(Terrain::layerset setname = LAYERSET_SURFACE);

     /** Reads a file into a dtTerrain::HeightField.
      *  @param shift If it is true, then the data is shifted by SHRT_MIN downwards to convert
      *  unsigned data into the signed format of the heightfield class.
      * @returns 0 if an error occured.
      */
     static dtTerrain::HeightField* ReadHF(std::string fname, bool shift);

     /** Applies the original mask to the terrain from the .xml description.
      * Overwrites any changes to the current mask.
      */
     void ApplyDefaultMask();

     /** Get the mask image for edit. You have to call dirty() on this image if you modify it to apply the changes.
      * @return The image pointer to the terrain mask. Initially this image holds the default mask.
      */
     osg::Image* GetMaskImage();

     /** Get the recolor image for edit. You have to call dirty() on this image if you modify it to apply the changes.
      * @return The image pointer to the recolor image. Initially this image holds the default recolor image. 
      */
     osg::Image* GetRecolorImage();

     /** This function projects the given x_orig and y_orig coordinates, assuming they are taking the range of
      * [0..x_max] and [0..y_max] into surface coordinates.
      * @return x_trans the transformed x coordinate,
      * @return y_trans the transformed y coordinate.
      */
     void ProjectCoordinates(float x_orig, float y_orig, float x_max, float y_max,
                                      float& x_trans, float& y_trans);

     /** This function projects the real-world x_orig and y_orig coordinates
      * into the range [0..x_max] and [0..y_max] based on their relative position compared to the size of the surface.
      * @return x_trans the transformed x coordinate,
      * @return y_trans the transformed y coordinate.
      */
     void InverseProjectCoordinates(float x_orig, float y_orig, float x_max, float y_max,
                                      float& x_trans, float& y_trans);

     /** This function projects the real-world x and y coordinates
      * into integer layer coordinates.
      * @param layer_coord_x the transformed x coordinate (outgoing),
      * @param layer_coord_y the transformed y coordinate (outgoing).
      */
     void GetLayerCoordinatesAt(float x, float y,
         unsigned int & layer_coord_x, unsigned int & layer_coord_y);

     /** This function projects the real-world x and y coordinates
      * into float (subpixel) layer coordinates.
      * @param layer_coord_x the transformed x coordinate (outgoing),
      * @param layer_coord_y the transformed y coordinate (outgoing).
      */  
     void GetSubPixelLayerCoordinatesAt(float x, float y, 
         float & layer_coord_x, float & layer_coord_y);

     /** This function returns the active layers at a given position.
      * @return The set of active layers, encoded in bits (0th bit: 1st layer, 1st bit: 2nd layer etc.).
      */
     long unsigned int GetLayerBitsAt(float x, float y);

     /** Overloaded version to use raw layer coordinates
      */
     long unsigned int GetLayerBitsAt(int x, int y);

     /** This function returns the layer index for a given layer name.
      * It can be used to check whether a given layer is present in the mask given back by
      * GetLayerBitsAt().
      * @param The name of the layer as defined in the map .xml file.
      * @return The layer index if found, -1 otherwise.
      */
     signed int GetLayerIndex(std::string layer_name);

     /** Get yield modifier of bullets at the given coordinate.
      * @return The yield modifier. If no yield modifier
      * was defined, it returns 1.
      */
     float GetBulletYieldAt(float x, float y);

     /** Get yield modifier of shells at the given coordinate.
      * @return The yield modifier. If no yield modifier
      * was defined, it returns 1.
      */
     float GetShellYieldAt(float x, float y);

     /** Returns the name of the layer which affects sensors at the given coordinate.
      * @return The name of the layer, or "none" if no affecting layers in that position.
      */
     std::string GetLayerSensorsAt(float x, float y);

     /** Adds a drawable to the scene.
      */
     void AddDrawable(dtCore::DeltaDrawable* drawable);

     void addTerrainOverlayChild(dtCore::Object *obj);
     void removeTerrainOverlayChild(dtCore::Object *obj);

     /** adds a path to the list of primitives.
      * if the "vertices" member of "path" is not empty but "agg_path" is empty, we fill
      * agg_path with a plain line, vertices taken from "vertices" which is a plain std::vector of vertex types.
      * @ return A unique handle (identifier) which can be used to refer to the path when deleting.
      * Returns 0 if the generated handle was already existing (should happen only if sb inserted billions of paths...)
      */
     unsigned int AddPath(PathPrimitive path, bool redraw = false);

     /** Removes a path from the list of primitives.
      * @return True if erasing was successful, false if the element did not exist. 
      */
     bool RemovePath(unsigned int handle, bool redraw = false);
     
     /** Returns a path data for the given handle. 
      */
     PathPrimitive& GetPath(unsigned int handle) { return m_Paths[handle]; }
     
     /** Redraws all stored primitives.
      */
     void RedrawPrimitives();
     
     /** Redraws primitives which are in transition.
      */
     void UpdatePrimitives();

     /** Selects primitive to transition.
      */
     void TransitionPrimitives(unsigned int handle, PathPrimitive target);

     /** Stops all animation.
      */
     void StopTransitions();

  protected:

     /**
      * Destructor.
      */
     virtual ~Terrain();

     /** This function creates and fills the m_DefaultTerrainMaskImage and the m_LayerBits array
      * based on the data in m_TerrainLayers. Executed only once during construction.
      */
     virtual void ProcessLayers();

     /** this functions sets the dash types in the passed class of AGG
      */
     template <class dashclass> void SetDash(dashclass & dash, dash_type type);
                   
     float m_LOSPostSpacing; ///<used to samples points for LOS calculations

     // the heightfield of the surface
     dtCore::RefPtr<dtTerrain::HeightField> m_HeightField;

     // stores the texture index associated with a position
     dtCore::RefPtr<dtTerrain::HeightField> m_AttributeMap;

     // stores all the textures (filled in the constructor)
     std::vector< dtCore::RefPtr<osg::Texture2D> > m_TextureArray;
     // stores associated index for each layer
     std::vector<unsigned int> m_TextureIndices;

     // the overall texture applied to the terrain
     dtCore::RefPtr<osg::Texture2D> m_TerrainTexture;

     // the dynamic mask applied to the terrain
     dtCore::RefPtr<osg::Texture2D> m_TerrainMask;

     // the texture storing recoloring information
     dtCore::RefPtr<osg::Texture2D> m_RecolorTexture;
     
     // the core image associated with the terrain mask. This is set up by the .xml
     // description and cannot be modified by class function calls.
     dtCore::RefPtr<osg::Image> m_DefaultTerrainMaskImage;

     // the actual image associated with the terrain mask. This is the one applied to the texture and
     // can be manipulated by function calls.
     dtCore::RefPtr<osg::Image> m_TerrainMaskImage;

     // the core image associated with recoloring. This is set up by the .xml
     // description and cannot be modified by class function calls.
     dtCore::RefPtr<osg::Image> m_DefaultRecolorImage;

     // the actual image associated with recoloring. This is the one applied to the texture and
     // can be manipulated by function calls.
     dtCore::RefPtr<osg::Image> m_RecolorImage;

     // objects:
     // surface object
     dtCore::RefPtr<SurfaceObject> m_Surface;

     short m_HFMaxValue, m_HFMinValue, m_DefAltitude;

     std::string m_MapName, m_MinimapFileName;

     // this array holds all the terrain layers
     std::vector< dtCore::RefPtr<TerrainLayer> > m_TerrainLayers;

     // this is the overlay object
     dtCore::RefPtr<dtCore::Object> m_TerrainOverlay;

     // this array stores booleans in long int for each layer
     std::vector< std::vector<unsigned long int> > m_LayerBits;
          
     // these are the objects required for the AGG library
     
     typedef agg::pixfmt_rgba32 pixfmt;
     typedef agg::renderer_base<agg::pixfmt_rgba32> agg_base;

     agg::rendering_buffer m_AggRenderBuffer;
     pixfmt m_AggPixFormat;
     agg_base m_AggRenderBase;
     
     // this is the container storing the line primitives
     std::map<unsigned int,PathPrimitive> m_Paths;
     typedef std::map<unsigned int,PathPrimitive>::iterator path_iterator;
     
     // this is storing the actual handle counter. New paths will be assigned a key from this number
     // for later referral.
     unsigned int m_HandleCounter;
     
     std::map<unsigned int, PathPrimitive> m_OriginalPaths;
     std::map<unsigned int, PathPrimitive> m_TargetPaths;
     std::map<unsigned int, unsigned int> m_TransitionTimers;
};

#endif // TERRAIN_H_
