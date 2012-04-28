#include "UnitConfigHandler.h"

#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wextra"

#include <iostream>
#include <dtUtil/stringutils.h>
#include <dtUtil/xercesutils.h>

#include <xercesc/sax2/XMLReaderFactory.hpp>

#pragma GCC diagnostic warning "-Wextra"

UnitConfigHandler::UnitConfigHandler()
{
}

UnitConfigHandler::~UnitConfigHandler()
{
}

   void UnitConfigHandler::characters(const XMLCh* const chars, const unsigned int length)
   {
      /*if (mCurrentElement == UnitConfigSchema::LIBRARY_PATH)
      {
         std::string path(dtUtil::XMLStringConverter(chars).ToString());
         mConfigData.LIBRARY_PATHS.push_back(path);
      }*/
   }

   void UnitConfigHandler::endDocument() {}
   void UnitConfigHandler::endElement(const XMLCh* const uri,const XMLCh* const localname,const XMLCh* const qname) {}
   void UnitConfigHandler::ignorableWhitespace(const XMLCh* const chars, const unsigned int length) {}
   void UnitConfigHandler::processingInstruction(const XMLCh* const target, const XMLCh* const data) {}
   void UnitConfigHandler::setDocumentLocator(const XERCES_CPP_NAMESPACE_QUALIFIER Locator* const locator) {}
   void UnitConfigHandler::startDocument() {}
   void UnitConfigHandler::startPrefixMapping(const	XMLCh* const prefix,const XMLCh* const uri) {}
   void UnitConfigHandler::endPrefixMapping(const XMLCh* const prefix) {}
   void UnitConfigHandler::skippedEntity(const XMLCh* const name) {}

   ///\todo use ApplicationConfigSchema for attribute and node name searches.
   ///\todo optimize out string and data copies by just using the default applicatioinconfigdata struct
   void UnitConfigHandler::startElement(const XMLCh* const uri,
                                               const XMLCh* const localname,
                                               const XMLCh* const qname,
                                               const XERCES_CPP_NAMESPACE_QUALIFIER Attributes& attrs)
   {
      typedef dtUtil::AttributeSearch::ResultMap RMap;

      dtUtil::XMLStringConverter stringConv(localname);

      mCurrentElement = stringConv.ToString();

      if( mCurrentElement == UnitConfigSchema::UNIT )
      {
         // search for specific named attributes, append the list of searched strings
         dtUtil::AttributeSearch attrsearch;
         // perform the search
         RMap results = attrsearch( attrs );

         // if the named attributes were found, use them, else use defaults
         RMap::iterator iter = results.find(UnitConfigSchema::NAME);
         if( iter != results.end() )
            mConfigData.unit_name = iter->second;

         iter = results.find(UnitConfigSchema::FLYING);
         if( iter != results.end() )
            mConfigData.flying = dtUtil::ToType<bool>((iter->second).c_str());

         iter = results.find(UnitConfigSchema::ARMOUR);
         if( iter != results.end() )
            mConfigData.armour = dtUtil::ToType<double>((iter->second).c_str());

      }
      else if( mCurrentElement == UnitConfigSchema::MOVEMENT )
      {
         dtUtil::AttributeSearch attrsearch;
         RMap results = attrsearch( attrs );
         RMap::iterator iter;

         iter = results.find(UnitConfigSchema::MAX_SPEED);
         if( iter != results.end() )
         	mConfigData.movement.max_speed = dtUtil::ToType<float>((iter->second).c_str());

         iter = results.find(UnitConfigSchema::AVERAGE_SPEED);
         if( iter != results.end() )
        	mConfigData.movement.average_speed = dtUtil::ToType<float>((iter->second).c_str());

         iter = results.find(UnitConfigSchema::SLOW_SPEED);
         if( iter != results.end() )
         	mConfigData.movement.slow_speed = dtUtil::ToType<float>((iter->second).c_str());

         iter = results.find(UnitConfigSchema::TURNING_RATE);
         if( iter != results.end() )
         	mConfigData.movement.turning_rate = dtUtil::ToType<float>((iter->second).c_str());

         iter = results.find(UnitConfigSchema::SLOWING_DIST);
         if( iter != results.end() )
         	mConfigData.movement.slowing_dist = dtUtil::ToType<float>((iter->second).c_str());

         iter = results.find(UnitConfigSchema::STOPPING_DIST);
         if( iter != results.end() )
         	mConfigData.movement.stopping_dist = dtUtil::ToType<float>((iter->second).c_str());
      }
      else if( mCurrentElement == UnitConfigSchema::PHYSICS )
      {
         dtUtil::AttributeSearch attrsearch;
         RMap results = attrsearch( attrs );
         RMap::iterator iter;

         iter = results.find(UnitConfigSchema::MASS);
         if( iter != results.end() )
         	mConfigData.physics.mass = dtUtil::ToType<float>((iter->second).c_str());

         iter = results.find(UnitConfigSchema::MAX_FORCE);
         if( iter != results.end() )
         	mConfigData.physics.max_force = dtUtil::ToType<float>((iter->second).c_str());

         iter = results.find(UnitConfigSchema::SUSPENSION_ERP);
         if( iter != results.end() )
         	mConfigData.physics.suspension_erp = dtUtil::ToType<float>((iter->second).c_str());

         iter = results.find(UnitConfigSchema::SUSPENSION_CFM);
         if( iter != results.end() )
         	mConfigData.physics.suspension_cfm = dtUtil::ToType<float>((iter->second).c_str());

         iter = results.find(UnitConfigSchema::TURN_ERP);
         if( iter != results.end() )
         	mConfigData.physics.turn_erp = dtUtil::ToType<float>((iter->second).c_str());

         iter = results.find(UnitConfigSchema::TURN_CFM);
         if( iter != results.end() )
         	mConfigData.physics.turn_cfm = dtUtil::ToType<float>((iter->second).c_str());
      }
      else if ( mCurrentElement == UnitConfigSchema::WHEELS )
      {
         dtUtil::AttributeSearch attrsearch;
         RMap results = attrsearch( attrs );
         RMap::iterator iter;

         iter = results.find(UnitConfigSchema::MASS);
         if( iter != results.end() )
         	mConfigData.wheels.mass = dtUtil::ToType<float>((iter->second).c_str());

         iter = results.find(UnitConfigSchema::X);
         if( iter != results.end() )
         	mConfigData.wheels.x = dtUtil::ToType<float>((iter->second).c_str());

         iter = results.find(UnitConfigSchema::Y_FRONT);
         if( iter != results.end() )
         	mConfigData.wheels.y_front = dtUtil::ToType<float>((iter->second).c_str());

         iter = results.find(UnitConfigSchema::Y_REAR);
         if( iter != results.end() )
         	mConfigData.wheels.y_rear = dtUtil::ToType<float>((iter->second).c_str());

         iter = results.find(UnitConfigSchema::Z);
         if( iter != results.end() )
         	mConfigData.wheels.z = dtUtil::ToType<float>((iter->second).c_str());
      }
      else if ( mCurrentElement == UnitConfigSchema::FIRING )
      {
         dtUtil::AttributeSearch attrsearch;
         RMap results = attrsearch( attrs );
         RMap::iterator iter;

         iter = results.find(UnitConfigSchema::RELOAD);
         if( iter != results.end() )
         	mConfigData.firing.reload = dtUtil::ToType<float>((iter->second).c_str());

         iter = results.find(UnitConfigSchema::TURRET);
         if( iter != results.end() )
         	mConfigData.firing.turret = dtUtil::ToType<float>((iter->second).c_str());

         iter = results.find(UnitConfigSchema::BARREL);
         if( iter != results.end() )
         	mConfigData.firing.barrel = dtUtil::ToType<float>((iter->second).c_str());
      }
      else if ( mCurrentElement == UnitConfigSchema::PROJECTILE )
      {
         dtUtil::AttributeSearch attrsearch;
         RMap results = attrsearch( attrs );
         RMap::iterator iter;

         iter = results.find(UnitConfigSchema::MASS);
         if( iter != results.end() )
         	mConfigData.projectile.mass = dtUtil::ToType<float>((iter->second).c_str());

         iter = results.find(UnitConfigSchema::RADIUS);
         if( iter != results.end() )
         	mConfigData.projectile.radius = dtUtil::ToType<float>((iter->second).c_str());

         iter = results.find(UnitConfigSchema::VELOCITY);
         if( iter != results.end() )
         	mConfigData.projectile.velocity = dtUtil::ToType<float>((iter->second).c_str());

         iter = results.find(UnitConfigSchema::YIELD);
         if( iter != results.end() )
         	mConfigData.projectile.yield = dtUtil::ToType<float>((iter->second).c_str());

         iter = results.find(UnitConfigSchema::RANGE);
         if( iter != results.end() )
         	mConfigData.projectile.range = dtUtil::ToType<float>((iter->second).c_str());

         iter = results.find(UnitConfigSchema::TYPE);
         if( iter != results.end() )
         	mConfigData.projectile.type = iter->second;
      }
      else if ( mCurrentElement == UnitConfigSchema::OBJECT || mCurrentElement == UnitConfigSchema::CHARACTER )
      {
         dtUtil::AttributeSearch attrsearch;
         RMap results = attrsearch( attrs );
         RMap::iterator iter;

         int num = 1;
         MeshData meshData;
         meshData.type = mCurrentElement;

         iter = results.find(UnitConfigSchema::NAME);
         if( iter != results.end() )
         	meshData.name = iter->second;

         iter = results.find(UnitConfigSchema::MESH);
         if( iter != results.end() )
         	meshData.mesh = iter->second;

         iter = results.find(UnitConfigSchema::NUMBER);
         if( iter != results.end() )
         	num = dtUtil::ToType<int>((iter->second).c_str());

         for (int i=0; i<num; i++) {
         	mConfigData.meshes.push_back(meshData);
         }
       }
       else if ( mCurrentElement == UnitConfigSchema::SENSORS )
       {
          dtUtil::AttributeSearch attrsearch;
          RMap results = attrsearch( attrs );
          RMap::iterator iter;

          iter = results.find(UnitConfigSchema::LAYER_ID);
          std::string layer_id = iter->second;

          iter = results.find(UnitConfigSchema::RANGE);
          if( iter != results.end() )
                 mConfigData.sensors.range[layer_id] = dtUtil::ToType<float>((iter->second).c_str());

          iter = results.find(UnitConfigSchema::VISIBILITY);
          if( iter != results.end() )
                 mConfigData.sensors.visibility[layer_id] = dtUtil::ToType<float>((iter->second).c_str());
       }

   }
