#ifndef TERRAINLAYER_H_
#define TERRAINLAYER_H_

#include "../common.h"
#pragma GCC diagnostic ignored "-Wextra"

#include <vector>
#include <boost/dynamic_bitset.hpp>
#include <dtCore/base.h>
#include <dtCore/object.h>
#include <osg/Geometry>
#include <dtTerrain/heightfield.h>

#pragma GCC diagnostic warning "-Wextra"

class Terrain;
class QDomElement;
class Cell;

///////////////////////////////////////////////////////////////////////
/** Base class which is capable of creating bitmasks from .png files.
 */
class DT_CORE_EXPORT TerrainLayer : public dtCore::Base
  {
    DECLARE_MANAGEMENT_LAYER(TerrainLayer)
    
    public:
      
        // this type stores the type of the graphical features in the layer.
        // it will be converted into objects and a corresponding layer on the map. 
        enum feature_type {
          SQUARE
        };
      
        /** Constructs a layer from an XML element.
         */
        TerrainLayer( QDomElement & e, const std::string & root_path );
        
        /** Returns the mask's value at (x,y) (in image coordinates).
         */
        bool GetMaskAt( unsigned int x, unsigned int y)
          {
            return m_BitMask[x][y];
          }

        /** Sets the mask's value at (x,y) (in image coordinates).
         */
        void SetMaskAt( unsigned int x, unsigned int y, bool value)
          {
            m_BitMask[x][y] = value;
          }
        
        unsigned int GetWidth() { return m_Width; }
        unsigned int GetHeight() { return m_Height; }
                
        /** Returns the color associated with this layer. This color will be applied to
         * the terrain texture.
         * @return The RGB color of the mask.
         */
        unsigned char* GetColor() { return m_Color; }

        /** Returns the recolor value associated with this layer. If it is 1, the color influences
         * the luminance of the real terrain surface, otherwise it entirely replaces it.
         * @return The value of recoloring.
         */
        unsigned char GetRecolor() { return m_Recolor; }

        /** Get yield modifier of bullets in this layer.
         * @return The yield modifier. If negative, then no yield modifier
         * was defined for this layer.
         */        
        float GetBulletYield() { return m_BulletYield; }

        /** Get yield modifier of shells in this layer.
         * @return The yield modifier. If negative, then no yield modifier
         * was defined for this layer.
         */        
        float GetShellYield() { return m_ShellYield; }

        /** Returns whether this layer influences sensors (visibility).
         */
        bool DoesAffectSensors() { return m_AffectsSensors; }
        
        /** Creates the objects associated with this layer or returns 0 if no object are associated.
         *  
         *  @param terrain The terrain object to get data required for the generation from
         *  (e.g. the height of the surface at a given coordinate).
         *  @return the pointer to the dtCore::Object which is created, or 0 if no object were created.
         * 
         */
        void CreateObjects(Terrain& terrain);
                
        /** Calculates the positions of a grid with the given density superimposed on the mask.
         * @return (x,y) pairs of positions (in image coordinates).
         */
        std::vector< std::pair<unsigned int,unsigned int> > GetVolumetricFeaturePositions(float density);
        
        /** This function adjusts the heightfield to a flat surface below the features.
         * It also accepts a scaling factor to align the size difference of the heightfield and the layer
         * (should be size_of_hf/size_of_layer)
         */
        void AdjustHeightfield(dtTerrain::HeightField & hf, float ratio);
        
    protected:
        
        /** Accepts a filename (with path) to create the bitmask from.
         * @param filename The filename with full path to read from.
         * @return True if the loading was successful.
         */
        bool Load(std::string filename);

        /** Processes the bitmask and transforms it into a point-like mask based on the center of boxes found.
         * Boxes should not overlap (results may be ambiguous). Width and height of boxes is arbitrary,
         * but must be at least 3 in both directions. New points will be placed at the center of them.
         */
        void ProcessBoxFeatures();

        osg::Geometry* createOrthogonalQuads( const osg::Vec3& pos,
                                                   float w, float h, osg::Vec4ub color );
        osg::Node* createGraph(Cell* cell, osg::StateSet* stateset,
                                  std::vector<dtCore::RefPtr<dtCore::Object> > & objects);
        
        virtual ~TerrainLayer()
          { DeregisterInstance(this);}
        
    private:
           
        unsigned int m_Width, m_Height;
        
        // bitmask
        std::vector< boost::dynamic_bitset<> > m_BitMask;
        
        // color associated with this layer. The alpha channel stores its strength
        // related to other textures: if 0, then it is mixed with other textures, if >0, then 
        // the terrain textures are re-colored into this value (see the fragment shader).
        unsigned char m_Color[4];
                
        // this store the recoloring value. 1 means the layer changes the luminance of the underlying
        // terrain (like a shadow), 0 means it entirely replaces the underlying terrain texture.
        unsigned char m_Recolor;
                
        // this stores the filenames for the objects
        std::vector<std::string> m_ObjectFileList;

        // this stores the scaling of the objects
        std::vector<float> m_Scales;

        // images for quads
        std::string m_SideImage;
        std::string m_TopImage;
        
        // This stores the mean of the number of objects per layer position
        float m_Mean;

        // This stores the sigma of the number of objects per layer position
        float m_Sigma;    
        
        // this stores how much layer coordinates to group in an osg transform group
        unsigned int m_GroupSize;
        
        // this stores the new feature size after processing the layer map
        unsigned int m_NewFeatureSize;
        
        // this stores the centers of the features
        std::vector<osg::Vec2> m_ListOfFeatures;
        
        // yields of different ammunitions valid in this layer. If negative, then 
        // no yield modifier is defined for this layer
        float m_BulletYield, m_ShellYield;
        
        // is this layer affecting sensors?
        bool m_AffectsSensors;
};

/** This class generates a tree structure of points, given the maximum
 * number of points in each cell.
 */
class Cell : public osg::Referenced
{
public:
    typedef std::vector< osg::ref_ptr<Cell> > CellList;
    typedef std::vector< osg::Vec3 > PointList;

    Cell():_parent(0) {}
    Cell(osg::BoundingBox& bb):_parent(0), _bb(bb) {}
    
    void addCell(Cell* cell) { cell->_parent=this; _cells.push_back(cell); }

    void addPoint(osg::Vec3 point) { _points.push_back(point); }
    
    void addPoints(const PointList& points) { _points.insert(_points.end(),points.begin(),points.end()); }
    
    void computeBound();
    
    bool contains(const osg::Vec3& position) const { return _bb.contains(position); }
    
    bool divide(unsigned int maxNumPointsPerCell=10);
    
    bool devide(bool xAxis, bool yAxis, bool zAxis);
    
    void bin();

    Cell*               _parent;
    osg::BoundingBox    _bb;
    CellList            _cells;
    PointList            _points;
};

#endif /*TERRAINLAYER_H_*/
