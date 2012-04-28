#include "ScenarioHandler.h"
#include <dtUtil/stringutils.h>
#include <dtUtil/xercesutils.h>

#include <xercesc/sax2/XMLReaderFactory.hpp>

ScenarioHandler::ScenarioHandler()
{
	mScenarioData = new ScenarioData();
}

ScenarioHandler::~ScenarioHandler()
{
}

   void ScenarioHandler::characters(const XMLCh* const chars, const unsigned int length) 
   {
      /*if (mCurrentElement == UnitConfigSchema::LIBRARY_PATH)
      {
         std::string path(dtUtil::XMLStringConverter(chars).ToString());
         mConfigData.LIBRARY_PATHS.push_back(path);
      }*/
   }
   
   void ScenarioHandler::endDocument() {}
   void ScenarioHandler::endElement(const XMLCh* const uri,const XMLCh* const localname,const XMLCh* const qname) {
	   dtUtil::XMLStringConverter stringConv(localname);
	         
	   mCurrentElement = stringConv.ToString();
	   
	   if( mCurrentElement == "scenario" ) {
		   //end of the document
		   mScenarioData->numInstances = mInstanceIDs.size();
	   }
   }
   void ScenarioHandler::ignorableWhitespace(const XMLCh* const chars, const unsigned int length) {}
   void ScenarioHandler::processingInstruction(const XMLCh* const target, const XMLCh* const data) {}
   void ScenarioHandler::setDocumentLocator(const XERCES_CPP_NAMESPACE_QUALIFIER Locator* const locator) {}
   void ScenarioHandler::startDocument() {}
   void ScenarioHandler::startPrefixMapping(const	XMLCh* const prefix,const XMLCh* const uri) {}
   void ScenarioHandler::endPrefixMapping(const XMLCh* const prefix) {}
   void ScenarioHandler::skippedEntity(const XMLCh* const name) {}
   
   ///\todo use ApplicationConfigSchema for attribute and node name searches.
   ///\todo optimize out string and data copies by just using the default applicatioinconfigdata struct
   void ScenarioHandler::startElement(const XMLCh* const uri,
                                               const XMLCh* const localname,
                                               const XMLCh* const qname,
                                               const XERCES_CPP_NAMESPACE_QUALIFIER Attributes& attrs)
   {
      typedef dtUtil::AttributeSearch::ResultMap RMap;
      
      dtUtil::XMLStringConverter stringConv(localname);
      
      mCurrentElement = stringConv.ToString();
         
      if( mCurrentElement == "scenario" )
      {
         // search for specific named attributes, append the list of searched strings
         dtUtil::AttributeSearch attrsearch;  
         // perform the search
         RMap results = attrsearch( attrs );
   
         RMap::iterator iter = results.find("map");
         if( iter != results.end() )
            mScenarioData->mapName = iter->second;

         iter = results.find("time_limit");
         if( iter != results.end() )
            mScenarioData->timeLimit = dtUtil::ToType<int>((iter->second).c_str());

         iter = results.find("goal_time");
            if( iter != results.end() )
               mScenarioData->goalTime = dtUtil::ToType<int>((iter->second).c_str());

        iter = results.find("controller");
         if( iter != results.end() )
            mScenarioData->controllerId = dtUtil::ToType<int>((iter->second).c_str());

        iter = results.find("game");
         if( iter != results.end() )
            mScenarioData->gameType = iter->second;

        iter = results.find("sim_time");
         if( iter != results.end() )
            mScenarioData->simTime = dtUtil::ToType<float>((iter->second).c_str());

        iter = results.find("speedup");
		 if( iter != results.end() )
		    mScenarioData->speedup = dtUtil::ToType<float>((iter->second).c_str());

		iter = results.find("auto_fire");
		 if( iter != results.end() )
			mScenarioData->autoFire = dtUtil::ToType<bool>((iter->second).c_str());
      }
      else if( mCurrentElement == "side" )
      {
         dtUtil::AttributeSearch attrsearch;
         RMap results = attrsearch( attrs );
         RMap::iterator iter;
         
         mCurrentSide = new ScenarioSideData();
         //record side in scenario data structure
         mScenarioData->sides.push_back(mCurrentSide);
   
         iter = results.find("name");
         if( iter != results.end() )
         	mCurrentSide->setSide(iter->second);
      }
      else if( mCurrentElement == "unit" )
      {
         dtUtil::AttributeSearch attrsearch;
         RMap results = attrsearch( attrs );
         RMap::iterator iter;
         
         mCurrentUnit = new ScenarioUnitData();
         mCurrentSide->units.push_back(mCurrentUnit);
   
         iter = results.find("type");
         if( iter != results.end() )
         	mCurrentUnit->setType(iter->second);
   
         iter = results.find("number");
         if( iter != results.end() ) {
         	mCurrentUnit->number = dtUtil::ToType<int>((iter->second).c_str());
         	mCurrentSide->total += mCurrentUnit->number;
         }
         	
         iter = results.find("instance");
         if( iter != results.end() ) {
           
                if (iter->second == "local")
                  {
                    mCurrentUnit->instance = theApp->getInstanceID();
                  }
                else
                  {
                    mCurrentUnit->instance = dtUtil::ToType<int>((iter->second).c_str());
                  }
         	//add instance id if its not already there
         	bool found = false;
         	for (unsigned int i=0; i < mInstanceIDs.size(); i++) {
         		if (mInstanceIDs.at(i) == mCurrentUnit->instance) {
         			found = true;
         			break;
         		}
         	}
         	if (!found) {
         		mInstanceIDs.push_back(mCurrentUnit->instance);
         	}
         }
      }
      else if ( mCurrentElement == "pos" || mCurrentElement == "startpos" || mCurrentElement == "endpos")
      {
         dtUtil::AttributeSearch attrsearch;
         RMap results = attrsearch( attrs );
         RMap::iterator iter;
         
         osg::Vec2 pos;
   
         iter = results.find("x");
         if( iter != results.end() )
         	pos[0] = dtUtil::ToType<float>((iter->second).c_str());
         	
         iter = results.find("y");
         if( iter != results.end() )
         	pos[1] = dtUtil::ToType<float>((iter->second).c_str());
         	
         if (mCurrentElement == "pos") {
         	mCurrentUnit->addPosition(pos);
         	mCurrentSide->total++;
         } else {
         	mCurrentUnit->addStartOrEndPosition(pos);
         }
      } 
   }
