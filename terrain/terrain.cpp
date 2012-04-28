//////////////////////////////////////////////////////////////////////

#include "../common.h"
#pragma GCC diagnostic ignored "-Wextra"

#include <QtXml/QtXml>
#undef emit // needed because of conflict with OSG

#include <prefix/dtcoreprefix-src.h>
#include <dtCore/globals.h>
#include <dtCore/light.h>
#include <dtUtil/mathdefines.h>
#include <dtUtil/matrixutil.h>
#include <dtUtil/log.h>
#include <dtTerrain/heightfield.h>
#include <vector>
#include <cmath>
#include <cstring>
#include <agg2/agg_renderer_primitives.h>
#include <agg2/agg_basics.h>
#include <agg2/agg_rendering_buffer.h>
#include <agg2/agg_rasterizer_scanline_aa.h>
#include <agg2/agg_conv_stroke.h>
#include <agg2/agg_conv_dash.h>
#include <agg2/agg_conv_curve.h>
#include <agg2/agg_conv_contour.h>
#include <agg2/agg_conv_smooth_poly1.h>
#include <agg2/agg_conv_marker.h>
#include <agg2/agg_arrowhead.h>
#include <agg2/agg_vcgen_markers_term.h>
#include <agg2/agg_scanline_u.h>
#include <agg2/agg_renderer_scanline.h>
#include <agg2/agg_path_storage.h>

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
#include "boost/lexical_cast.hpp"

#pragma GCC diagnostic warning "-Wextra"

#include "terrain.h"
#include "TerrainLayer.h"
#include "../HTapp.h"
#include "SurfaceObject.h"

#undef Status
#undef Bool
#include <QtCore/QFile>
#include <QtXml/QtXml>
#include <osgSim/OverlayNode>

// 60 frames
const float TerrTransFrames = 15;

using namespace dtCore;

IMPLEMENT_MANAGEMENT_LAYER(Terrain)

Terrain::Terrain(const std::string & filename, bool use_overlay_node) :
  m_LOSPostSpacing(25.f), m_HandleCounter(1)
  {     
    RegisterInstance(this);
    
    // load in the .xml which explains how to construct the terrain
    QDomDocument doc( "Map" );
    
    LOG_DEBUG("Parsing map: " + filename + "...");
    
    std::string map_path = dtCore::FindFileInPathList( "maps/" + filename + "/" );
    map_path += "/";
    if (map_path.empty())
      {
         LOG_ERROR("Could not find map directory to load: " + filename );                 
      }    
    
    std::string map_xml_file = map_path + filename + ".xml";
    QString fn( map_xml_file.c_str() );
    QFile file( fn );
    if( !file.open( QFile::ReadOnly ) )
      LOG_ERROR("Cannot open map definition file: " + map_xml_file);
    
    QString errormsg;
    int errorline;
    
    if( !doc.setContent( &file, &errormsg, &errorline ) )
    {
      file.close();
      LOG_ERROR("Cannot parse map definition file " + filename + " at line " + 
          boost::lexical_cast<std::string>(errorline) + ": " + errormsg.toStdString() );
    }
    file.close();

    QDomElement root = doc.documentElement();
      
    bool found_map = false;
    float horizontal_size, min_alt, max_alt, segment_size;
    unsigned int segment_divisions;
    std::string heightmap_fname, attributemap_fname, texturemap_fname;

    std::map<unsigned int, unsigned int> texture_IDs;

    if( !root.isNull() )
    {
      if( root.tagName() == "map" )
      {
        found_map = true;
        
        m_MapName = root.attribute("name").toStdString();
        horizontal_size = boost::lexical_cast<float>((root.attribute("horizontal_size")).toStdString());
        min_alt = boost::lexical_cast<float>((root.attribute("min_alt")).toStdString());
        max_alt = boost::lexical_cast<float>((root.attribute("max_alt")).toStdString());
        segment_size = boost::lexical_cast<float>((root.attribute("segment_size")).toStdString());
        segment_divisions = boost::lexical_cast<unsigned int>((root.attribute("segment_divisions")).toStdString());
        m_MinimapFileName = dtCore::FindFileInPathList( "maps/" + filename + "/" +
            root.attribute("minimap").toStdString() );
        heightmap_fname = dtCore::FindFileInPathList( "maps/" + filename + "/" +
            root.attribute("height_map").toStdString() );
        attributemap_fname = dtCore::FindFileInPathList( "maps/" + filename + "/" +
            root.attribute("attribute_map").toStdString() );
        texturemap_fname = dtCore::FindFileInPathList( "maps/" + filename + "/" +
            root.attribute("texture_map").toStdString() );                
        
        // now walk through layers and create them
        QDomElement e = root.firstChild().toElement();
        while (!e.isNull() )
          {
            if( e.tagName() == "texture" )
              {
                std::string texture_fname = e.attribute("file").toStdString();
                unsigned int texture_id = boost::lexical_cast<unsigned int>((e.attribute("id")).toStdString());
                osg::Image* image = osgDB::readImageFile(dtCore::FindFileInPathList( texture_fname ));
                dtCore::RefPtr<osg::Texture2D> texturep = new osg::Texture2D(image);        
                texturep->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
                texturep->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);

                // store the texture for passing it to SurfaceObject
                texture_IDs[texture_id] = m_TextureArray.size();
                m_TextureArray.push_back( texturep );
              }

            if( e.tagName() == "layer" )
              {
                std::string layer_type = e.attribute("type").toStdString();

                // parse the layers

                if (layer_type == "mask")
                  {
                   m_TerrainLayers.push_back( new TerrainLayer(e, map_path) );
                   
                   // check that all the layers have the same size
                   if (m_TerrainLayers.size() > 0)
                     {
                       unsigned int first_width = m_TerrainLayers.front()->GetWidth();
                       unsigned int first_height = m_TerrainLayers.back()->GetHeight();
                       unsigned int actual_width = m_TerrainLayers.front()->GetWidth();
                       unsigned int actual_height = m_TerrainLayers.back()->GetHeight();
                                              
                       if ( first_width != actual_width || first_height != actual_height)
                         {
                           LOG_ERROR("Terrain layer size mismatch. Layers should have the same size.")
                         }
                     }
                  }
              }
            e = e.nextSibling().toElement();
          }
        }
     }
        
    if (!found_map)
      {
        LOG_ERROR("Could not find map description in the .xml file.");
        return;
      }
    
    LOG_DEBUG("Added " + boost::lexical_cast<std::string>(m_TextureArray.size()) +" textures.");
    LOG_DEBUG("Added " + boost::lexical_cast<std::string>(m_TerrainLayers.size()) +" terrain mask layers.");

    // read the heightfield and the attribute map
    m_HeightField = ReadHF(heightmap_fname, true);
    if (!m_HeightField)
      {
        LOG_ERROR("Cannot read heightfield.")
      }
    m_AttributeMap = ReadHF(attributemap_fname, false);
    if (!m_AttributeMap)
      {
        LOG_ERROR("Cannot read attribute map.")
      }
    
    unsigned int width = m_HeightField->GetNumColumns();
    unsigned int height = m_HeightField->GetNumRows();
    unsigned int i,j;

    // re-assign indices of the attribute map to hold indices into m_TextureArray
    for (i = 0; i < width; i++)
      for (j = 0; j < height; j++)
        {
          m_AttributeMap->SetHeight(i, j, texture_IDs[m_AttributeMap->GetHeight(i, j)] );
        }
        
    // read the texture map
    osg::Image* image = osgDB::readImageFile( texturemap_fname );
    m_TerrainTexture = new osg::Texture2D(image);        
    m_TerrainTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    m_TerrainTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    
    // this function could be moved inside the constructor, but GCC and GDB cannot debug
    // in the constructor properly...
    ProcessLayers();

    // create a copy of the default image for the texture (this allocates the mask image as well)
    m_TerrainMaskImage = dynamic_cast<osg::Image*>(m_DefaultTerrainMaskImage->clone(osg::CopyOp::DEEP_COPY_ALL));
          
    m_TerrainMask = new osg::Texture2D(m_TerrainMaskImage.get());
    m_TerrainMask->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    m_TerrainMask->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    m_TerrainMask->setDataVariance(osg::Object::DYNAMIC);

    // create a copy of the default image for the texture (this allocates the mask image as well)
    m_RecolorImage = dynamic_cast<osg::Image*>(m_DefaultRecolorImage->clone(osg::CopyOp::DEEP_COPY_ALL));

    m_RecolorTexture = new osg::Texture2D(m_RecolorImage.get());
    m_RecolorTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::REPEAT);
    m_RecolorTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::REPEAT);
    m_RecolorTexture->setDataVariance(osg::Object::DYNAMIC);

    // create the AGG buffer
    int mask_width = m_TerrainMaskImage->s();
    int mask_height = m_TerrainMaskImage->t();
    m_AggRenderBuffer.attach(m_TerrainMaskImage->data(), mask_width, mask_height, 4*mask_width);
    m_AggPixFormat.attach(m_AggRenderBuffer);
    m_AggRenderBase.attach(m_AggPixFormat);

    // create the overlay object if required
    if (use_overlay_node)
      {
        m_TerrainOverlay = new dtCore::Object("terrainOverlay");
      }
    else
      {
        m_TerrainOverlay = 0;
      }

    unsigned int act_layer;
    
    // adjust heightmap to form base of singular features (like protected areas)
    /*for (act_layer = 0; act_layer < m_TerrainLayers.size(); act_layer++)
      {
        float ratio = (float)(width)/(float)(m_TerrainLayers[act_layer]->GetWidth());
        m_TerrainLayers[act_layer]->AdjustHeightfield(*m_HeightField, ratio);
      }*/
    
    // now create the surface object
    m_Surface = new SurfaceObject(m_HeightField, m_AttributeMap,
        m_TextureArray, m_TerrainTexture.get(), m_TerrainMask.get(),
        m_RecolorTexture.get(), m_TerrainOverlay.get() );
    
    m_Surface->SetHorizontalSize(horizontal_size);
    m_Surface->SetMinAlt(min_alt);
    m_Surface->SetMaxAlt(max_alt);
    m_Surface->SetSegmentDivisions(segment_divisions);
    m_Surface->SetSegmentSize(segment_size);
        
    // create layer objects
    for (act_layer = 0; act_layer < m_TerrainLayers.size(); act_layer++)
      {
        m_TerrainLayers[act_layer]->CreateObjects(*this);
      }
    
    // add the surface as an object
    theApp->GetScene()->AddDrawable( m_Surface.get() );
    
    // sync name
    m_Surface->GetOSGNode()->setName( GetName() );    
    
    /*    PathPrimitive B1_COA1;
     B1_COA1.vertices.push_back(osg::Vec2(-190,252));
     B1_COA1.vertices.push_back(osg::Vec2(257,987));
     B1_COA1.vertices.push_back(osg::Vec2(1091,1479));
     B1_COA1.color = agg::rgba(1, 1, 1, 1);
     B1_COA1.smooth = 1;
     B1_COA1.filled = false;
     B1_COA1.start_marker = false;
     B1_COA1.end_marker = true;
     B1_COA1.closed = false;
     B1_COA1.dash = LINE;
     B1_COA1.width = 1;
     terrain->AddPath(B1_COA1,true);    

     PathPrimitive B1_COA2;
     B1_COA2.vertices.push_back(osg::Vec2(-190,252));
     B1_COA2.vertices.push_back(osg::Vec2(363,133));
     B1_COA2.vertices.push_back(osg::Vec2(1127,497));
     B1_COA2.vertices.push_back(osg::Vec2(1747,734));
     B1_COA2.color = agg::rgba(0, 0.5, 1, 1);
     B1_COA2.smooth = 1;
     B1_COA2.filled = false;
     B1_COA2.start_marker = false;
     B1_COA2.end_marker = true;
     B1_COA2.closed = false;
     B1_COA2.dash = NORMAL;
     B1_COA2.width = 2;
     terrain->AddPath(B1_COA2,true);    
     
     PathPrimitive B1_COA3;
     B1_COA3.vertices.push_back(osg::Vec2(-190,252));
     B1_COA3.vertices.push_back(osg::Vec2(-1251,-80));
     B1_COA3.color = agg::rgba(1, 1, 1, 1);
     B1_COA3.smooth = 1;
     B1_COA3.filled = false;
     B1_COA3.start_marker = false;
     B1_COA3.end_marker = true;
     B1_COA3.closed = false;
     B1_COA3.dash = DOT;
     B1_COA3.width = 1;
     terrain->AddPath(B1_COA3,true);    

     PathPrimitive B2_COA1;
     B2_COA1.vertices.push_back(osg::Vec2(-500,-144));
     B2_COA1.vertices.push_back(osg::Vec2(81,-386));
     B2_COA1.vertices.push_back(osg::Vec2(846,36));
     B2_COA1.color = agg::rgba(1, 1, 1, 1);
     B2_COA1.smooth = 1;
     B2_COA1.filled = false;
     B2_COA1.start_marker = false;
     B2_COA1.end_marker = true;
     B2_COA1.closed = false;
     B2_COA1.dash = LINE;
     B2_COA1.width = 1;
     terrain->AddPath(B2_COA1,true);  

     PathPrimitive B2_COA2;
     B2_COA2.vertices.push_back(osg::Vec2(-500,-144));
     B2_COA2.vertices.push_back(osg::Vec2(-71,-341));
     B2_COA2.vertices.push_back(osg::Vec2(262,-803));
     B2_COA2.vertices.push_back(osg::Vec2(846,-987));
     B2_COA2.vertices.push_back(osg::Vec2(1935,-867));
     B2_COA2.vertices.push_back(osg::Vec2(2100,-143));
     B2_COA2.color = agg::rgba(0, 0.5, 1, 1);
     B2_COA2.smooth = 1;
     B2_COA2.filled = false;
     B2_COA2.start_marker = false;
     B2_COA2.end_marker = true;
     B2_COA2.closed = false;
     B2_COA2.dash = NORMAL;
     B2_COA2.width = 2;
     terrain->AddPath(B2_COA2,true);  

     PathPrimitive B2_COA3;
     B2_COA3.vertices.push_back(osg::Vec2(-500,-144));
     B2_COA3.vertices.push_back(osg::Vec2(-415,246));
     B2_COA3.vertices.push_back(osg::Vec2(-979,-625));
     B2_COA3.color = agg::rgba(1, 1, 1, 1);
     B2_COA3.smooth = 1;
     B2_COA3.filled = false;
     B2_COA3.start_marker = false;
     B2_COA3.end_marker = true;
     B2_COA3.closed = false;
     B2_COA3.dash = DOT;
     B2_COA3.width = 1;
     terrain->AddPath(B2_COA3,true);  

     PathPrimitive R1_COA1;
     R1_COA1.vertices.push_back(osg::Vec2(2131,830));
     R1_COA1.vertices.push_back(osg::Vec2(2217,184));
     R1_COA1.vertices.push_back(osg::Vec2(1684,337));
     R1_COA1.color = agg::rgba(1, 0.2, 0, 1);
     R1_COA1.smooth = 1;
     R1_COA1.filled = false;
     R1_COA1.start_marker = false;
     R1_COA1.end_marker = true;
     R1_COA1.closed = false;
     R1_COA1.dash = NORMAL;
     R1_COA1.width = 2;
     terrain->AddPath(R1_COA1,true);
     
     terrain->RedrawPrimitives();

     // RemovePath(handle1,true);*/    
    
}

Terrain::~Terrain()
{
   DeregisterInstance(this);
}

void Terrain::ProcessLayers()
  {
    // prepare the default terrain mask from the layers
    unsigned int mask_width = m_TerrainLayers.front()->GetWidth();
    unsigned int mask_height = m_TerrainLayers.front()->GetHeight();
    unsigned int i,j;
    
    assert(mask_width != 0);
    assert(mask_height != 0);
    
    m_DefaultTerrainMaskImage = new osg::Image();        
    m_DefaultTerrainMaskImage->allocateImage(mask_width, mask_height, 1, 
    GL_RGBA, GL_UNSIGNED_BYTE);

    m_DefaultRecolorImage = new osg::Image();
    m_DefaultRecolorImage->allocateImage(mask_width, mask_height, 1, GL_ALPHA, 
    GL_UNSIGNED_BYTE);
    
    unsigned char* act_color;
    unsigned char def_color[4] =
      { 255, 255, 255, 0 };
    unsigned int act_layer;
    unsigned char act_recolor = 0;
    
    // fill with default color first and resize layer bits
    
    m_LayerBits.resize(mask_width);
    
    for (i = 0; i < mask_width; i++)
      {
        m_LayerBits[i].resize(mask_height);
        for (j = 0; j < mask_height; j++)
          {
            memcpy(m_DefaultTerrainMaskImage->data(i,j),def_color,4);
            memcpy(m_DefaultRecolorImage->data(i, j), &act_recolor, 1);
          }    
      }
    
    for (act_layer = 0; act_layer < m_TerrainLayers.size(); act_layer++)
      {
        act_color = m_TerrainLayers[act_layer]->GetColor();                
        act_recolor = m_TerrainLayers[act_layer]->GetRecolor();
    
        for (i = 0; i < mask_width; i++)
          for (j = 0; j < mask_height; j++)
            {
                if ( m_TerrainLayers[act_layer]->GetMaskAt(i,j) )
                  {
                    // copy colors; later layers overwrite previous ones
                    memcpy(m_DefaultTerrainMaskImage->data(i,j),act_color,4);
                  memcpy(m_DefaultRecolorImage->data(i, j), &act_recolor, 1);
                    
                    // set its bit to high
                    m_LayerBits[i][j] |= (1l << act_layer);
                  }
            }
      }
}

void Terrain::AddDrawable(dtCore::DeltaDrawable* drawable)
  {
    theApp->GetScene()->AddDrawable( drawable );
  }

float Terrain::GetHeight(float x, float y, Terrain::layerset setname)
  {
    if (setname == Terrain::LAYERSET_SURFACE)
      {
        return m_Surface->GetHeight(x,y);
      }
    else
      {
        // TODO add additional options for sets
        return 0;        
      }
  }

void Terrain::GetNormal(float x, float y, osg::Vec3& normal, layerset setname)
  {
    if (setname == LAYERSET_SURFACE)
      {
        m_Surface->GetNormal(x,y,normal);
      }
    else
      {
        // TODO add additional options
        normal = osg::Vec3(0,0,1);        
      }
  }

bool Terrain::IsClearLineOfSight(const osg::Vec3& pointOne,
    const osg::Vec3& pointTwo, layerset setname)
      {
        if (setname == LAYERSET_SURFACE)
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
     else
       {
         // TODO add additional layers
         return true;
       }
}   

dtTerrain::HeightField* Terrain::ReadHF(std::string fname, bool shift)
  {
    osg::Image* image;
    dtTerrain::HeightField* hf = 0;
    image = osgDB::readImageFile(fname);
    
    if ( image )
      {
                
        if (image->getPixelSizeInBits() != 16)
          {
            LOG_ERROR("Error while reading heightfield: image format mismatch (should be 16-bit greyscale).")
          }        
        else if ( image->getPixelFormat() != GL_LUMINANCE )
          {
            LOG_ERROR("Error while reading heightfield: image format mismatch (should be GL_LUMINANCE).")
          }
        else
          {
            hf = new dtTerrain::HeightField( image->s(), image->t() );
            unsigned int width = hf->GetNumColumns();
            unsigned int height = hf->GetNumRows();
            unsigned int i,j;

            // copy values
            for (i = 0; i < width; i++)
              for (j = 0; j < height; j++)
                {
                  float value;
                  if (shift)
                    value = *( (unsigned short*)(image->data(i, j)) )
                        - SHRT_MIN;
                  else
                    value = *( (short*)(image->data(i,j)) );
                  
                  hf->SetHeight(i, j, (signed short)value );
                }            
          }
        
      }
    else
      {
        LOG_ERROR("Cannot load image.")
      }    
    
    return hf;
  }

osg::BoundingBox Terrain::GetBB(layerset setname)
  {
    if (setname == LAYERSET_SURFACE)
      {
        return m_Surface->GetBB();
      }
    else
      {
        osg::BoundingBox terrBB(-FLT_MAX, -FLT_MAX, -FLT_MAX, FLT_MAX, FLT_MAX, 
        FLT_MAX);
        return terrBB;
      }
  }

void Terrain::ApplyDefaultMask()
  {
    m_TerrainMaskImage->copySubImage(0, 0, 0, m_DefaultTerrainMaskImage.get());
    m_TerrainMaskImage->dirty();
  }

/** Get the mask image for edit.
 * @return The image pointer to the terrain mask. Initially this image holds the default mask. 
 */
osg::Image* Terrain::GetMaskImage()
{
  return m_TerrainMaskImage.get();
}

osg::Image* Terrain::GetRecolorImage()
  {
    return m_RecolorImage.get();
  }

void Terrain::ProjectCoordinates(float x_orig, float y_orig, float x_max,
    float y_max, float& x_trans, float& y_trans)
  {
    float horiz_size = m_Surface->GetHorizontalSize();
    
    // currently consider only the surface object...
    x_trans = (x_orig/x_max - 0.5)*horiz_size;
    y_trans = (y_orig/y_max - 0.5)*horiz_size;
  }

void Terrain::InverseProjectCoordinates(float x_orig, float y_orig,
    float x_max, float y_max, float& x_trans, float& y_trans)
  {
    float horiz_size = m_Surface->GetHorizontalSize();
    
    // currently consider only the surface object...
    x_trans = (x_orig/horiz_size + 0.5)*x_max;
    y_trans = (y_orig/horiz_size + 0.5)*y_max;
  }

void Terrain::addTerrainOverlayChild(dtCore::Object *obj)
  {
    m_TerrainOverlay->AddChild(obj);
  }

void Terrain::removeTerrainOverlayChild(dtCore::Object *obj)
  {
    if (m_TerrainOverlay != 0)
    m_TerrainOverlay->RemoveChild(obj);
  }

long unsigned int Terrain::GetLayerBitsAt(float x, float y)
  {
    float x_trans, y_trans;
    InverseProjectCoordinates(x, y, m_LayerBits.size(), m_LayerBits[0].size(),
        x_trans, y_trans);
    return m_LayerBits[(unsigned int)x_trans][(unsigned int)y_trans];
  }

long unsigned int Terrain::GetLayerBitsAt(int x, int y)
  {
    return m_LayerBits[x][y];
  }

signed int Terrain::GetLayerIndex(std::string layer_name)
  {
    // determine the layer index
    // this can be used to test whether a layer is present in the layer set bitmask or not
    unsigned int k;
    for (k=0; k<m_TerrainLayers.size(); k++)
      {
        if (m_TerrainLayers[k]->GetName() == layer_name)
          {
            return k;
          }
      }
    // not found
    return -1;
  }

float Terrain::GetHorizontalSize() const
  {
    return m_Surface->GetHorizontalSize();
  }

float Terrain::GetBulletYieldAt(float x, float y)
  {
    signed int act_layer;
    float layer_yield;

    unsigned int layer_coord_x, layer_coord_y;    
    GetLayerCoordinatesAt(x, y, layer_coord_x, layer_coord_y);
    
    for (act_layer = m_TerrainLayers.size()-1; act_layer >= 0; act_layer--)
      {
        // return the last existing layer modifier
        if ( m_TerrainLayers[act_layer]->GetMaskAt(layer_coord_x, layer_coord_y) )
          {
            layer_yield = m_TerrainLayers[act_layer]->GetBulletYield();
            if ( layer_yield > 0 )
              {
                return layer_yield;
              }
          }
      }
    // return default
    return 1;
  }

float Terrain::GetShellYieldAt(float x, float y)
  {
    signed int act_layer;
    float layer_yield;

    unsigned int layer_coord_x, layer_coord_y;    
    GetLayerCoordinatesAt(x, y, layer_coord_x, layer_coord_y);
    
    for (act_layer = m_TerrainLayers.size()-1; act_layer >= 0; act_layer--)
      {
        // return the last existing layer modifier
        if ( m_TerrainLayers[act_layer]->GetMaskAt(layer_coord_x, layer_coord_y) )
          {
            layer_yield = m_TerrainLayers[act_layer]->GetShellYield();
            if ( layer_yield > 0 )
              {
                return layer_yield;
              }
          }
      }
    // return default
    return 1;
  }

std::string Terrain::GetLayerSensorsAt(float x, float y)
  {
    signed int act_layer;

    unsigned int layer_coord_x, layer_coord_y;    
    GetLayerCoordinatesAt(x, y, layer_coord_x, layer_coord_y);
    
    for (act_layer = m_TerrainLayers.size()-1; act_layer >= 0; act_layer--)
      {
        // return the last layer's name affecting the sensors
        if (m_TerrainLayers[act_layer]->GetMaskAt(layer_coord_x, layer_coord_y)
            && m_TerrainLayers[act_layer]->DoesAffectSensors() )
          {
            return m_TerrainLayers[act_layer]->GetName();
          }
      }
    // return default
    std::string layer_name = "none";
    return layer_name;
  }

void Terrain::GetLayerCoordinatesAt(float x, float y,
    unsigned int & layer_coord_x, unsigned int & layer_coord_y)
  {
    // again assume that all layers has the same size - spare some CPU
    unsigned int mask_width = m_TerrainLayers.front()->GetWidth();
    unsigned int mask_height = m_TerrainLayers.front()->GetHeight();

    float x_trans, y_trans;
    
    InverseProjectCoordinates(x, y, mask_width, mask_height, x_trans, y_trans);
    
    x_trans = HT_MIN(mask_width-1, x_trans);
    y_trans = HT_MIN(mask_height-1, y_trans);

    x_trans = HT_MAX(0, x_trans);
    y_trans = HT_MAX(0, y_trans);
    
    layer_coord_x = round(x_trans);
    layer_coord_y = round(y_trans);    
  }

void Terrain::GetSubPixelLayerCoordinatesAt(float x, float y,
    float & layer_coord_x, float & layer_coord_y)
  {
    // again assume that all layers has the same size - spare some CPU
    unsigned int mask_width = m_TerrainLayers.front()->GetWidth();
    unsigned int mask_height = m_TerrainLayers.front()->GetHeight();

    float x_trans, y_trans;

    InverseProjectCoordinates(x, y, mask_width, mask_height, x_trans, y_trans);

    layer_coord_x = HT_MIN(mask_width-1, x_trans);
    layer_coord_y = HT_MIN(mask_height-1, y_trans);

    layer_coord_x = HT_MAX(0, x_trans);
    layer_coord_y = HT_MAX(0, y_trans);
  }

unsigned int Terrain::AddPath(PathPrimitive path, bool redraw)
  {
    // if vertices is not empty but agg_path is empty, we assume we have to copy it into the path
    if (path.vertices.size() > 0 && path.agg_path.total_vertices() == 0)
      {
        // convert the coordinates into mask coordinates with subpixel accuracy
        std::vector<osg::Vec2> transformed_vertices;
        for (unsigned int i = 0; i < path.vertices.size(); i++)
          {
            osg::Vec2 tr_vert;
            GetSubPixelLayerCoordinatesAt(path.vertices[i].x(),
                path.vertices[i].y(), tr_vert.x(), tr_vert.y());
            transformed_vertices.push_back(tr_vert);
          }

        path.agg_path.move_to(transformed_vertices[0].x(),
            transformed_vertices[0].y());

        for (unsigned int j = 1; j<transformed_vertices.size(); j++)
          {
            path.agg_path.line_to(transformed_vertices[j].x(),
                transformed_vertices[j].y());
          }

        if (path.closed)
          {
            path.agg_path.close_polygon();
          }
      }

    std::pair <std::map<unsigned int,PathPrimitive>::iterator,bool> return_value;
    std::pair<unsigned int,PathPrimitive> new_value(m_HandleCounter, path);
    return_value = m_Paths.insert(new_value);
    if (!return_value.second)
      {
        LOG_DEBUG("Tried to insert a path with an already existing handle.");
        return 0;
      }
    else
      {
        m_HandleCounter++;
        // for overflow, but doubt that anybody will insert 4 billion paths...
        if (m_HandleCounter == 0)
          m_HandleCounter++;
        if (redraw)
          RedrawPrimitives();

        // give back the new handle
        return new_value.first;
      }
  }

bool Terrain::RemovePath(unsigned int handle, bool redraw)
  {
    bool result = (bool)(m_Paths.erase(handle));
    if (redraw)
      RedrawPrimitives();
    return result;
  }

void Terrain::RedrawPrimitives()
  {
    // first clear the mask
    ApplyDefaultMask();

    // m_AggRenderBase.clear(agg::rgba(1, 1, 1));

    // create the scanline rasterizer
    agg::rasterizer_scanline_aa<> ras;
    agg::scanline_u8 sl;
    agg::line_cap_e cap = agg::butt_cap;

    // walk through all lines and render them
    for (path_iterator it = m_Paths.begin(); it != m_Paths.end(); it++)
      {
        // create the arrowhead
        double k =:: pow(it->second.width, 0.7);
        agg::arrowhead ah;

        if (it->second.start_marker || it->second.end_marker)
          {
            if (it->second.start_marker)
              {
                ah.tail(1*k,1.5*k, 3*k, 5*k);
              }
            if (it->second.end_marker)
              {
                ah.head(4*k, 4*k, 3*k, 2*k);
              }
          }

        if (it->second.smooth == 0)
          {
            if (it->second.filled)
              {
                ras.add_path(it->second.agg_path);
              }
            else
              {
                agg::conv_dash<stl_path_storage, agg::vcgen_markers_term> dash(it->second.agg_path);
                SetDash(dash, it->second.dash);

                agg::conv_stroke<agg::conv_dash<stl_path_storage, agg::vcgen_markers_term> > line(dash);
                line.line_cap(cap);
                line.width(it->second.width);
                ras.add_path(line);

                if (it->second.start_marker || it->second.end_marker)
                  {
                    agg::conv_marker<agg::vcgen_markers_term, agg::arrowhead> arrow(dash.markers(), ah);
                    ras.add_path(arrow);
                  }
              }
          }
        else
          {
            agg::conv_smooth_poly1<stl_path_storage> smooth(it->second.agg_path);
            smooth.smooth_value(it->second.smooth);
            agg::conv_curve<agg::conv_smooth_poly1<stl_path_storage> > curve(smooth);
            if (it->second.filled)
              {
                ras.add_path(curve);
              }
            else
              {
                agg::conv_dash<agg::conv_curve<agg::conv_smooth_poly1<stl_path_storage> >, agg::vcgen_markers_term> dash(curve);
                agg::conv_stroke<agg::conv_dash<agg::conv_curve<agg::conv_smooth_poly1<stl_path_storage> >, agg::vcgen_markers_term> > stroke(dash);
                stroke.line_cap(cap);
                stroke.width(it->second.width);
                SetDash(dash, it->second.dash);
                ras.add_path(stroke);

                if (it->second.start_marker || it->second.end_marker)
                  {
                    agg::conv_marker<agg::vcgen_markers_term, agg::arrowhead> arrow(dash.markers(), ah);
                    ras.add_path(arrow);
                  }
              }
          }

        agg::render_scanlines_aa_solid(ras, sl, m_AggRenderBase, it->second.color);
      }

    // commit the new image 
    m_TerrainMaskImage->dirty();
  }

template <class dashclass> void Terrain::SetDash(dashclass & dash,
    dash_type type)
  {
    switch (type)
      {
    case NORMAL:
      dash.add_dash(10.0, 0.0);
      break;
    case DOT:
      dash.add_dash(5.0, 5.0);
      break;
    case LINE:
      dash.add_dash(20.0, 5.0);
      break;
    case DOT_LINE:
      dash.add_dash(20.0, 5.0);
      dash.add_dash(5.0, 5.0);
      break;
    case DOT_DOT_LINE:
      dash.add_dash(20.0, 5.0);
      dash.add_dash(5.0, 5.0);
      dash.add_dash(5.0, 5.0);
      break;
      }
    dash.dash_start(10);
  }

std::map<unsigned int, PathPrimitive> m_TargetPaths;
std::map<unsigned int, unsigned int> m_TransitionTimers;

void Terrain::UpdatePrimitives()
  {
    if (m_TransitionTimers.size() != 0)
      {
        std::map<unsigned int, unsigned int>::iterator map_it;

        for (map_it = m_TransitionTimers.begin(); map_it
            != m_TransitionTimers.end(); map_it++)
          {
            // linear transfer between the original and the target
            unsigned handle = (*map_it).first;

            // add -1 so the final state is the new state, thus no extra copy needed
            float act_phase = ((float)((*map_it).second) - 1) /TerrTransFrames;

            m_Paths[handle].color = m_TargetPaths[handle].color.gradient(
                m_OriginalPaths[handle].color, act_phase);

            m_Paths[handle].width = act_phase*m_OriginalPaths[handle].width + (1-act_phase)*m_TargetPaths[handle].width;

            m_TransitionTimers[handle]--;

            if (m_TransitionTimers[handle] <= 0) // expired, remove
              {
                m_OriginalPaths.erase(handle);
                m_TargetPaths.erase(handle);
                m_TransitionTimers.erase(handle);
              }

          }
        RedrawPrimitives();
      }
  }

void Terrain::TransitionPrimitives(unsigned int handle, PathPrimitive target)
  {
    m_OriginalPaths[handle] = m_Paths[handle];
    m_TargetPaths[handle] = target;
    m_TransitionTimers[handle] = TerrTransFrames;
  }

void Terrain::StopTransitions()
  {
    std::map<unsigned int, unsigned int>::iterator map_it;

    for (map_it = m_TransitionTimers.begin(); map_it
        != m_TransitionTimers.end(); map_it++)
      {    
        map_it->second = 1;
      }
    
    UpdatePrimitives();
  }
