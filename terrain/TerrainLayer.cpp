#include <png.h>
#include "TerrainLayer.h"
#include <dtUtil/log.h>
#include "boost/lexical_cast.hpp"
#include "boost/random/normal_distribution.hpp"
#include <boost/random/linear_congruential.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/random/uniform_real.hpp>
#include <boost/random/variate_generator.hpp>
#include <osg/Image>
#include <osg/Group>
#include <osg/MatrixTransform>
#include <osgDB/ReadFile>
#include <osg/AlphaFunc>
#include <osg/BlendFunc>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Material>
#include <osg/Math>
#include <osg/MatrixTransform>
#include <osg/StateSet>
#include <osg/Texture2D>
#include <osg/TexEnv>

#include "terrain.h"

#undef Status
#undef Bool
#include <QtCore/QFile>
#include <QtXml/QtXml>

#include "../HTAppBase.h"

///////////////////////////////////////////////////////////////////////

IMPLEMENT_MANAGEMENT_LAYER(TerrainLayer)

TerrainLayer::TerrainLayer( QDomElement & e, const std::string & root_path )
  {
    QDomElement ee = e.firstChild().toElement(); 
    while ( !ee.isNull() )
      {
        if ( ee.tagName() == "params")
          {
            SetName( ee.attribute("name").toStdString() );
            QStringList list = ee.attribute("color").split(" ", QString::SkipEmptyParts);
            if (list.size() != 4)
              {
                LOG_ERROR("Incorrent color string.");
              }
            for (unsigned int i = 0; i<4; i++)
              {
                float color = list.at(i).toFloat();
                m_Color[i] = (unsigned char)(255*color);
              }
            std::string recolor = ee.attribute("recolor").toStdString();
            if (!recolor.empty()) {
              float frecolor = boost::lexical_cast<float>( recolor );
              m_Recolor = (unsigned char)(255*frecolor);
            }

            std::string feature_size = ee.attribute("new_feature_size").toStdString();
            if (!feature_size.empty()) m_NewFeatureSize = boost::lexical_cast<unsigned int>( feature_size );
            
            std::string mask_file = ee.attribute("file").toStdString();            
            if (! Load( root_path + mask_file )) LOG_ERROR("Cannot load mask file.");

            std::string feature_type = ee.attribute("feature_type").toStdString();
            if (feature_type != "")
              {
                if (feature_type == "box")
                  {
                    ProcessBoxFeatures();
                  }
                else
                  {
                    LOG_ERROR("Unknown feature type.")
                  }
              }

            std::string bullet_yield = ee.attribute("bullet_yield").toStdString();
            if (!bullet_yield.empty())
              {
                m_BulletYield = boost::lexical_cast<float>( bullet_yield );
              }
            else
              {
                m_BulletYield = 1.0f;
              }

            std::string shell_yield = ee.attribute("shell_yield").toStdString();
            if (!shell_yield.empty())
              {
                m_ShellYield = boost::lexical_cast<float>( shell_yield );
              }
            else
              {
                m_ShellYield = -1.0f;
              }

            std::string affects_sensors = ee.attribute("affects_sensors").toStdString();
            if (!affects_sensors.empty())
              {
                m_AffectsSensors = boost::lexical_cast<bool>( affects_sensors );
              }
            else
              {
                m_AffectsSensors = false;
              }
          
          }
        if ( ee.tagName() == "obj_params")
          {
            m_Mean = boost::lexical_cast<float>( ee.attribute("mean").toStdString() );
            m_Sigma = boost::lexical_cast<float>( ee.attribute("sigma").toStdString() );
            m_GroupSize = boost::lexical_cast<unsigned int>( ee.attribute("group").toStdString() );
          }
        if ( ee.tagName() == "object")
          {
            // load the object name list
            std::string fname = ee.attribute("file").toStdString();
            if (!fname.empty())
              {
                m_ObjectFileList.push_back( fname );
              }
            else // its a quad
              {
                m_ObjectFileList.push_back("quad");
                
                m_SideImage = ee.attribute("side_image").toStdString();
                m_TopImage = ee.attribute("top_image").toStdString();
              }
            m_Scales.push_back( boost::lexical_cast<float>( ee.attribute("scale").toStdString() ) );
          }
        ee = ee.nextSibling().toElement();
      }      
  }
        
void TerrainLayer::CreateObjects(Terrain& terrain)
  {    
    // exit if no object was defined
    if ( m_ObjectFileList.empty() ) return;

    unsigned int i, j, k;
    
    std::vector< dtCore::RefPtr<dtCore::Object> > objects;
    
    dtCore::RefPtr<osg::Texture2D> tex_side;
    dtCore::RefPtr<osg::Texture2D> tex_top;
    dtCore::RefPtr<osg::StateSet> dstate;
    dtCore::RefPtr<osg::AlphaFunc> alphaFunc;
    
    // load all objects
    for (k = 0; k < m_ObjectFileList.size(); k++)
      {
        if ( m_ObjectFileList[k] != "quad" )
        {
          LOG_DEBUG("Loading " + m_ObjectFileList[k]);
          objects.push_back( new dtCore::Object );
          objects.back()->LoadFile(m_ObjectFileList[k]);
        }
        else
        {
          if ( m_ObjectFileList.size() > 1)
            {
              LOG_WARNING("Warning: you have defined both quads and objects for a layer. Only quads will be generated.");
            }
          
          // setup quad rendering state
          tex_side = new osg::Texture2D;
          tex_side->setWrap( osg::Texture2D::WRAP_S, osg::Texture2D::CLAMP );
          tex_side->setWrap( osg::Texture2D::WRAP_T, osg::Texture2D::CLAMP );
          tex_side->setImage(osgDB::readImageFile(m_SideImage));

          dstate = new osg::StateSet;
          dstate->setTextureAttributeAndModes(0, tex_side.get(), osg::StateAttribute::ON );
          dstate->setTextureAttribute(0, new osg::TexEnv );
          dstate->setAttributeAndModes( new osg::BlendFunc, osg::StateAttribute::ON );

/*          tex_top = new osg::Texture2D;
          tex_top->setWrap( osg::Texture2D::WRAP_S, osg::Texture2D::CLAMP );
          tex_top->setWrap( osg::Texture2D::WRAP_T, osg::Texture2D::CLAMP );
          tex_top->setImage(osgDB::readImageFile(m_TopImage));

          dstate->setTextureAttributeAndModes(1, tex_top.get(), osg::StateAttribute::ON );
          dstate->setTextureAttribute(1, new osg::TexEnv );
          dstate->setAttributeAndModes( new osg::BlendFunc, osg::StateAttribute::ON );
*/
          alphaFunc = new osg::AlphaFunc;
          alphaFunc->setFunction(osg::AlphaFunc::GEQUAL,0.05f);
          
          dstate->setAttributeAndModes( alphaFunc.get(), osg::StateAttribute::ON );
          dstate->setMode( GL_LIGHTING, osg::StateAttribute::OFF );
          dstate->setRenderingHint( osg::StateSet::TRANSPARENT_BIN );            
        }
      }    
    
    // walk through the layer's points and generate a point for each layer point as specified in the density 
    unsigned int num_objects;
    float x_coord, y_coord, z_coord, rand_tmp;
    float lower_left_x, lower_left_y, upper_right_x, upper_right_y, halfpoint_x, halfpoint_y;
    
    boost::minstd_rand generator(42u);
    boost::normal_distribution<> normd(m_Mean, m_Sigma);    
    boost::variate_generator<boost::minstd_rand&, boost::normal_distribution<> > normal_d(generator, normd);    

    // this vector stores the generated coordinates  
    std::vector<osg::Vec3> object_coords;
    
    // do not generate objects for the edge cells
    for (i = 1; i < m_Width-1; i++)
        for (j = 1; j < m_Height-1; j++)
          {
            if ( m_BitMask[i][j] == 1 )
              {
                // determine cell coordinates
                halfpoint_x = float(i) - 0.5f;
                halfpoint_y = float(j) - 0.5f;            
                terrain.ProjectCoordinates(halfpoint_x, halfpoint_y, m_Width, m_Height, lower_left_x, lower_left_y);
                halfpoint_x = float(i) + 0.5f;
                halfpoint_y = float(j) + 0.5f;            
                terrain.ProjectCoordinates(halfpoint_x, halfpoint_y, m_Width, m_Height, upper_right_x, upper_right_y);
    
                // determine the number of objects to create in this layer coordinate
                double randn = normal_d();
                
                num_objects = HT_MAX(0, floor(randn));
                
                for (k = 0; k < num_objects; k++)
                  {
                    // generate a coordinate at this layer coordinate
                    rand_tmp = random(0.0f, 1.0f);
                    x_coord = lower_left_x + rand_tmp*(upper_right_x - lower_left_x); 
                    rand_tmp = random(0.0f, 1.0f);
                    y_coord = lower_left_y + rand_tmp*(upper_right_y - lower_left_y);
                    z_coord = terrain.GetHeight(x_coord,y_coord);
                    
                    osg::Vec3 pos(x_coord, y_coord, z_coord);                
                    
                    object_coords.push_back(pos);
                  }
              }
          }

    LOG_DEBUG("Creating cell subdivision...");
    dtCore::RefPtr<Cell> cell = new Cell;
    cell->addPoints(object_coords);
    cell->divide(m_GroupSize);
    
    // walk the tree of the cells and add a group for each leaf, then generate
    // the objects for that leaf
  
    dtCore::RefPtr<osg::Group> group = new osg::Group;
    group->addChild(createGraph(cell.get(), dstate.get(), objects));

    osg::Group* scene_node = theApp->GetScene()->GetSceneNode();
    
    scene_node->addChild(group.get());
  }

bool TerrainLayer::Load( std::string filename )
  {
    osg::Image* image = osgDB::readImageFile( filename );
    
    if ( image )
      {
        // std::cout << GetName() << ", pixel size: " << image->getPixelSizeInBits() << std::endl;
        // std::cout << GetName() << ", pixel format: " << image->getPixelFormat() << std::endl;
        if (image->getPixelSizeInBits() != 32)
          {
            LOG_ERROR("Error while reading heightfield: image format mismatch (should be 32 bits per pixel).")
            return false;
          }        
        else if ( image->getPixelFormat() != GL_RGBA )
          {
            LOG_ERROR("Error while reading heightfield: image format mismatch (should be GL_RGBA).")
            return false;
          }
        else
          {
            m_Width = image->s();
            m_Height = image->t();
            m_BitMask.resize(m_Width);
            unsigned int i,j;

            // copy values; use the alpha channel to determine layout
            for (i = 0; i < m_Width; i++)
              {
                m_BitMask[i].resize(m_Height);
                for (j = 0; j < m_Height; j++)
                  {
                    unsigned char color[4];

                    void* act_pointer = image->data(i,j);
                    unsigned char* base_byte_pointer = (unsigned char*)act_pointer;
                    
                    color[0]= *base_byte_pointer;
                    color[1]= *(base_byte_pointer+1);
                    color[2]= *(base_byte_pointer+2);
                    color[3]= *(base_byte_pointer+3);

                    /* if (color[0] != 0 || color[1] != 0 || color[2] != 0 || color[3] != 0)
                      {
                        std::cout << GetName() << ", data at " << i << "," << j << ": " << 
                        (unsigned)color[0] << " " <<
                        (unsigned)color[1] << " " <<
                        (unsigned)color[2] << " " <<
                        (unsigned)color[3] << " " << std::endl;
                      }*/

                    // use the alpha channel to decide whether the mask is set
                    if (color[3] > 0)
                      {
                        SetMaskAt(i, j, true);
                      }
                    else
                      {
                        SetMaskAt(i,j, false);
                      }                  
                  }
              }
          }
        
      }
    else
      {
        LOG_ERROR("Cannot load image.")
        return false;
      }    
    
    return true;    
  }

std::vector< std::pair<unsigned int,unsigned int> > TerrainLayer::GetVolumetricFeaturePositions(float density)
{
  if (density < 1)
    LOG_ERROR("Density should be larger than 1.");
  
  std::vector< std::pair<unsigned int,unsigned int> > features;

  float x, y;
  unsigned int x_clamped, y_clamped;
  
  for (y=0; y<m_Height; y += density)
      for (x=0; x<m_Width; x+= density)
        {
          x_clamped = (unsigned int)x;
          y_clamped = (unsigned int)y;
          if (m_BitMask[y_clamped][x_clamped] == 1)
            {
              features.push_back(std::pair<unsigned int,unsigned int>(x_clamped, y_clamped) );
            }
        }
  
  return features;
}

void TerrainLayer::ProcessBoxFeatures()
  {
    // this is the target bitmask
    std::vector< boost::dynamic_bitset<> > new_bitmask;
    unsigned int i,j,k,l;

    // resize new bitmask
    new_bitmask.resize(m_Width);
    for (i = 0; i<m_Width; i++ )
        new_bitmask[i].resize(m_Height);
    
    // process old bitmask
    for (i = 0; i<m_Width; i++ )
      for (j = 0; j<m_Height; j++ )
        {
          // look for a box from this point (whether it is an upper left corner)
          bool is_upper_left_corner = (m_BitMask[i][j] == true);
          if ( i!=0 )
            if (m_BitMask[i-1][j] == true)
              is_upper_left_corner = false;

          if ( j!=0 )
            if (m_BitMask[i][j-1] == true)
              is_upper_left_corner = false;
          
          if (is_upper_left_corner)
            {                     
              // look into the two directions how long we walk up on points
              unsigned int x_line_length = 1, y_line_length = 1;
              bool line_end = false;
              while (!line_end)
                {
                  if (m_BitMask[i+x_line_length][j] == true)
                    {
                      x_line_length++;
                    }
                  else
                    {
                      line_end = true;
                    }
                }
              line_end = false;
              while (!line_end)
                {
                  if (m_BitMask[i][j+y_line_length] == true)
                    {
                      y_line_length++;
                    }
                  else
                    {
                      line_end = true;
                    }
                }
              
              if (x_line_length > 2 && y_line_length > 2)
                {
                  // a new square was found, set its feature in the new bitmask
                  
                  m_ListOfFeatures.push_back( osg::Vec2(i+floor(x_line_length/2), j+floor(y_line_length/2)) );
                  
                  for ( k = i+floor(x_line_length/2) - m_NewFeatureSize-1; 
                         k < i+floor(x_line_length/2) + m_NewFeatureSize-2; k++)
                    for ( l = j+floor(y_line_length/2) - m_NewFeatureSize-1;
                          l < j+floor(y_line_length/2) + m_NewFeatureSize-2; l++)
                          new_bitmask[k][l] = true;
                }
            }
        }
    
    // now copy the new bitmask onto the old one
    for (i = 0; i<m_Width; i++ )
      for (j = 0; j<m_Height; j++ )
        {
          m_BitMask[i][j] = new_bitmask[i][j];
        } 
  }

osg::Geometry* TerrainLayer::createOrthogonalQuads( const osg::Vec3& pos, float w, float h, osg::Vec4ub color )
{
    // set up the coords
    osg::Vec3Array& v = *(new osg::Vec3Array(8));
    osg::Vec2Array& t = *(new osg::Vec2Array(8));
    osg::Vec4ubArray& c = *(new osg::Vec4ubArray(1));
    
    float rotation = random(0.0f,osg::PI/2.0f);
    float sw = sinf(rotation)*w*0.5f;
    float cw = cosf(rotation)*w*0.5f;

    v[0].set(pos.x()-sw,pos.y()-cw,pos.z()+0.0f);
    v[1].set(pos.x()+sw,pos.y()+cw,pos.z()+0.0f);
    v[2].set(pos.x()+sw,pos.y()+cw,pos.z()+h);
    v[3].set(pos.x()-sw,pos.y()-cw,pos.z()+h);

    v[4].set(pos.x()-cw,pos.y()+sw,pos.z()+0.0f);
    v[5].set(pos.x()+cw,pos.y()-sw,pos.z()+0.0f);
    v[6].set(pos.x()+cw,pos.y()-sw,pos.z()+h);
    v[7].set(pos.x()-cw,pos.y()+sw,pos.z()+h);

    c[0] = color;

    t[0].set(0.0f,0.0f);
    t[1].set(1.0f,0.0f);
    t[2].set(1.0f,1.0f);
    t[3].set(0.0f,1.0f);

    t[4].set(0.0f,0.0f);
    t[5].set(1.0f,0.0f);
    t[6].set(1.0f,1.0f);
    t[7].set(0.0f,1.0f);

    osg::Geometry *geom = new osg::Geometry;

    geom->setVertexArray( &v );

    geom->setTexCoordArray( 0, &t );

    geom->setColorArray( &c );
    geom->setColorBinding( osg::Geometry::BIND_OVERALL );

    geom->addPrimitiveSet( new osg::DrawArrays(osg::PrimitiveSet::QUADS,0,8) );

    return geom;
}

osg::Node* TerrainLayer::createGraph(Cell* cell, osg::StateSet* stateset, std::vector<dtCore::RefPtr<dtCore::Object> > & objects)
{
    bool needGroup = !(cell->_cells.empty());
    bool needObjects = !(cell->_points.empty());
    
    osg::Group* transform_group = 0;
    osg::Group* group = 0;
    
    if (needObjects)
    {
        transform_group = new osg::Group;
        
        if (stateset == 0)
          {
            // we need objects
            // use a matrix node for them
            for(Cell::PointList::iterator itr=cell->_points.begin(); itr!=cell->_points.end(); ++itr)
            {
                unsigned int object_num = random(0,objects.size());
                if (object_num == objects.size() ) object_num = 0;

                osg::Vec3& point = *itr;
                osg::MatrixTransform* transform = new osg::MatrixTransform;
                transform->setMatrix(osg::Matrix::scale(m_Scales[object_num],m_Scales[object_num],m_Scales[object_num])*osg::Matrix::translate(point));
        
                transform->addChild( objects[object_num]->GetOSGNode() );
                transform_group->addChild(transform);
            }
          }
        else
          { // we need quads            
            for(Cell::PointList::iterator itr=cell->_points.begin(); itr!=cell->_points.end(); ++itr)
            {
                osg::Vec3& point = *itr;
                
                float size = random(0.5f, 1.0f);
                dtCore::RefPtr<osg::Geometry> geometry = createOrthogonalQuads(point,size*m_Scales[0],size*m_Scales[0],osg::Vec4ub(255,255,255,255));
                dtCore::RefPtr<osg::Geode> geode = new osg::Geode;
                geode->setStateSet(stateset);
                geode->addDrawable(geometry.get());
                transform_group->addChild(geode.get());        
            }
        }            
    }
    
    if (needGroup)
    {
        group = new osg::Group;
        for(Cell::CellList::iterator itr=cell->_cells.begin();
            itr!=cell->_cells.end();
            ++itr)
        {
            group->addChild(createGraph(itr->get(),stateset,objects));
        }
        
        if (transform_group) group->addChild(transform_group);
        
    }
    if (group) return group;
    else return transform_group;
}

void Cell::computeBound()
{
    _bb.init();
    for(CellList::iterator citr=_cells.begin();
        citr!=_cells.end();
        ++citr)
    {
        (*citr)->computeBound();
        _bb.expandBy((*citr)->_bb);
    }

    for(PointList::iterator titr=_points.begin();
        titr!=_points.end();
        ++titr)
    {
        _bb.expandBy( (*titr) );
    }
}

bool Cell::divide(unsigned int maxNumPointsPerCell)
{

    if (_points.size()<=maxNumPointsPerCell) return false;

    computeBound();

    float radius = _bb.radius();
    float divide_distance = radius*0.7f;
    if (devide((_bb.xMax()-_bb.xMin())>divide_distance,(_bb.yMax()-_bb.yMin())>divide_distance,(_bb.zMax()-_bb.zMin())>divide_distance))
    {
        // recusively divide the new cells till maxNumTreesPerCell is met.
        for(CellList::iterator citr=_cells.begin();
            citr!=_cells.end();
            ++citr)
        {
            (*citr)->divide(maxNumPointsPerCell);
        }
        return true;
   }
   else
   {
        return false;
   }
}

bool Cell::devide(bool xAxis, bool yAxis, bool zAxis)
{
    if (!(xAxis || yAxis || zAxis)) return false;

    if (_cells.empty())
        _cells.push_back(new Cell(_bb));

    if (xAxis)
    {
        unsigned int numCellsToDivide=_cells.size();
        for(unsigned int i=0;i<numCellsToDivide;++i)
        {
            Cell* orig_cell = _cells[i].get();
            Cell* new_cell = new Cell(orig_cell->_bb);

            float xCenter = (orig_cell->_bb.xMin()+orig_cell->_bb.xMax())*0.5f;
            orig_cell->_bb.xMax() = xCenter;
            new_cell->_bb.xMin() = xCenter;

            _cells.push_back(new_cell);
        }
    }

    if (yAxis)
    {
        unsigned int numCellsToDivide=_cells.size();
        for(unsigned int i=0;i<numCellsToDivide;++i)
        {
            Cell* orig_cell = _cells[i].get();
            Cell* new_cell = new Cell(orig_cell->_bb);

            float yCenter = (orig_cell->_bb.yMin()+orig_cell->_bb.yMax())*0.5f;
            orig_cell->_bb.yMax() = yCenter;
            new_cell->_bb.yMin() = yCenter;

            _cells.push_back(new_cell);
        }
    }

    if (zAxis)
    {
        unsigned int numCellsToDivide=_cells.size();
        for(unsigned int i=0;i<numCellsToDivide;++i)
        {
            Cell* orig_cell = _cells[i].get();
            Cell* new_cell = new Cell(orig_cell->_bb);

            float zCenter = (orig_cell->_bb.zMin()+orig_cell->_bb.zMax())*0.5f;
            orig_cell->_bb.zMax() = zCenter;
            new_cell->_bb.zMin() = zCenter;

            _cells.push_back(new_cell);
        }
    }

    bin();

    return true;

}

void Cell::bin()
{   
    // put trees in apprpriate cells.
    PointList pointsNotAssigned;
    for(PointList::iterator titr=_points.begin();
        titr!=_points.end();
        ++titr)
    {
        osg::Vec3 point = *titr;
        bool assigned = false;
        for(CellList::iterator citr=_cells.begin();
            citr!=_cells.end() && !assigned;
            ++citr)
        {
            if ((*citr)->contains(point))
            {
                (*citr)->addPoint(point);
                assigned = true;
            }
        }
        if (!assigned) pointsNotAssigned.push_back(point);
    }

    // put the unassigned trees back into the original local tree list.
    _points.swap(pointsNotAssigned);

    // prune empty cells.
    CellList cellsNotEmpty;
    for(CellList::iterator citr=_cells.begin();
        citr!=_cells.end();
        ++citr)
    {
        if (!((*citr)->_points.empty()))
        {
            cellsNotEmpty.push_back(*citr);
        }
    }
    _cells.swap(cellsNotEmpty);
}

void TerrainLayer::AdjustHeightfield(dtTerrain::HeightField & hf, float ratio)
  {
    unsigned int i,j,k;
    
    float size_of_area = (float)m_NewFeatureSize*ratio;
    
    for (k = 0; k < m_ListOfFeatures.size(); k++)
      {
        short central_height = hf.GetHeight(m_ListOfFeatures[k].x(), m_ListOfFeatures[k].y());
        
        for ( i = m_ListOfFeatures[k].x() - size_of_area; 
               i < m_ListOfFeatures[k].x() + size_of_area-1; i++)
          for ( j = m_ListOfFeatures[k].y() - size_of_area;
                j < m_ListOfFeatures[k].y() + size_of_area-1; j++)
                  hf.SetHeight(i,j,central_height);
      }
  }
