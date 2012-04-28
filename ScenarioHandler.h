#ifndef UNITCONFIGHANDLER_H_
#define UNITCONFIGHANDLER_H_

#include <xercesc/sax2/ContentHandler.hpp>  // for a base class
#include <xercesc/sax2/Attributes.hpp>      // for a parameter
#include <string>                           // for data members
#include "ScenarioData.h"    // for member

class ScenarioHandler : public XERCES_CPP_NAMESPACE_QUALIFIER ContentHandler
{
public:
	ScenarioHandler();
	virtual ~ScenarioHandler();
	// inherited pure virtual functions
         virtual void characters(const XMLCh* const chars, const unsigned int length);
         virtual void endDocument();
         virtual void endElement(const XMLCh* const uri,const XMLCh* const localname,const XMLCh* const qname);
         virtual void ignorableWhitespace(const XMLCh* const chars, const unsigned int length);
         virtual void processingInstruction(const XMLCh* const target, const XMLCh* const data);
         virtual void setDocumentLocator(const XERCES_CPP_NAMESPACE_QUALIFIER Locator* const locator);
         virtual void startDocument();
         virtual void startElement(const XMLCh* const uri,const XMLCh* const localname,const XMLCh* const qname, const XERCES_CPP_NAMESPACE_QUALIFIER Attributes& attrs);
         virtual void startPrefixMapping(const	XMLCh* const prefix,const XMLCh* const uri);
         virtual void endPrefixMapping(const XMLCh* const prefix);
         virtual void skippedEntity(const XMLCh* const name);
   
         ScenarioData *mScenarioData;
         ScenarioSideData *mCurrentSide;
         ScenarioUnitData *mCurrentUnit;
         std::vector<int> mInstanceIDs;
      private:
         std::string mCurrentElement;      
};

#endif /*UNITCONFIGHANDLER_H_*/
