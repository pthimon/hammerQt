#include <sstream>

#include <dtCore/transform.h>

#include "UnitRemote.h"
#include "../HTapp.h"

UnitRemote::UnitRemote(UnitInitPacket& init_packet, bool controlled,
    int client, int id) :
  Unit("", osg::Vec3(0, 0, 0), 0, RED, controlled, true, false, client, id)
  {
    // initialize based on init packet

    setName(init_packet.m_UniqueUnitID);

    if ( getName().find("Tank") )
      {
        parseUnitConfigFile("unitTank.xml");
      }
    else if ( getName().find("Soldier") )
      {
        parseUnitConfigFile("unitSoldier.xml");
      }

    m_ObjectModelNames = init_packet.m_ObjectModelNames;
    m_IsFlying = init_packet.m_IsFlying;
    setSide( (UnitSide)init_packet.m_Side );
    EnableDynamics(false);
  }

UnitRemote::~UnitRemote()
  {
  }

void UnitRemote::update()
  {
    // extract position and rotation from the position packets

    Unit::update();

    for (unsigned int i = 0; i < GetNumChildren(); i++)
      {
        osg::Vec3 newXYZ;
        osg::Vec3 newHPR;
        dtCore::Transform current_tr;
        std::ostringstream oss;
        oss << getName() << i;
        dtCore::Transformable* next_object =
            dynamic_cast<dtCore::Transformable*>(GetChild(i));

        theApp->GetPositionOfNetObject(oss.str(), newXYZ, newHPR);

        current_tr.SetTranslation(newXYZ);
        current_tr.SetRotation(newHPR);
        next_object->SetTransform(current_tr, Transformable::ABS_CS);
      }
  }

void UnitRemote::LoadMeshes()
  {
    // load all meshes according to the init packet

    for (unsigned int i = 0; i<m_ObjectModelNames.size(); i++)
      {
        // generate name for each object in the form of unique name + object number
        std::ostringstream ss;
        ss << GetName() << i;

        dtCore::RefPtr<dtCore::Object> object = new dtCore::Object(ss.str());

        if (!object->LoadFile(m_ObjectModelNames[i]))
          {
            LOG_ERROR("Cannot find remote model: " + m_ObjectModelNames[i]);
            return;
          }
        object->EnableDynamics(false);

        AddChild(object.get());
      }

  }

void UnitRemote::setVisible(bool visibility = true) {
	Unit::setVisible(visibility);

	SetActive(visibility);
}
