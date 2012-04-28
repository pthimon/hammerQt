#include "common.h"

#include <iostream>
#include <gnelib/PacketParser.h>
#include <gnelib/Buffer.h>

#include <sstream>

#include "packets.h"
#include "units/UnitTank.h"
#include "units/UnitSoldier.h"
#include "models/InverseModelFormation.h"

//our unique ID for this custom packet
const int PositionPacket::ID = GNE::PacketParser::MIN_USER_ID;


PositionPacket::PositionPacket():
GNE::Packet(ID)
{
}

PositionPacket::PositionPacket( osg::Vec3 xyz, osg::Vec3 hpr, const std::string &ownerID ):
GNE::Packet(ID)
{
   mXYZ = xyz;
   mHPR = hpr;
   mOwnerID = ownerID;
}

PositionPacket::PositionPacket( const PositionPacket &p ):
GNE::Packet(ID)
{
   mXYZ = p.mXYZ;
   mHPR = p.mHPR;
   mOwnerID = p.mOwnerID;
}

void PositionPacket::writePacket( GNE::Buffer &raw ) const
{
   GNE::Packet::writePacket(raw);
   raw << mXYZ[0];
   raw << mXYZ[1];
   raw << mXYZ[2];
   raw << mHPR[0];
   raw << mHPR[1];
   raw << mHPR[2];
   raw << mOwnerID;
}

void PositionPacket::readPacket( GNE::Buffer &raw)
{
   GNE::Packet::readPacket(raw);
   raw >> mXYZ[0];
   raw >> mXYZ[1];
   raw >> mXYZ[2];
   raw >> mHPR[0];
   raw >> mHPR[1];
   raw >> mHPR[2];
   raw >> mOwnerID;
}

//return the size in bytes
int PositionPacket::getSize() const
{
   return Packet::getSize()
   			+ sizeof(mXYZ[0]) + sizeof(mXYZ[1]) + sizeof(mXYZ[2])
   			+ sizeof(mHPR[0]) + sizeof(mHPR[1]) + sizeof(mHPR[2])
   			+ GNE::Buffer::getSizeOf(mOwnerID);
}

const int PlayerQuitPacket::ID = GNE::PacketParser::MIN_USER_ID + 1;

PlayerQuitPacket::PlayerQuitPacket( const std::string& playerID )
   :  GNE::Packet(ID),
      mPlayerID(playerID)
{
}

PlayerQuitPacket::PlayerQuitPacket()
:  GNE::Packet(ID)
{
}

PlayerQuitPacket::PlayerQuitPacket( const PlayerQuitPacket& p )
   :  GNE::Packet(ID),
      mPlayerID(p.mPlayerID)
{
}

void PlayerQuitPacket::writePacket( GNE::Buffer &raw ) const
{
   GNE::Packet::writePacket(raw);
   raw << mPlayerID;
}

void PlayerQuitPacket::readPacket( GNE::Buffer &raw )
{
   GNE::Packet::readPacket(raw);
   raw >> mPlayerID;
}

int PlayerQuitPacket::getSize() const
{
   return Packet::getSize() + sizeof(mPlayerID);
}

/////////// InitializingPacket ////////////////

const int InitializingPacket::ID = GNE::PacketParser::MIN_USER_ID + 2;

InitializingPacket::InitializingPacket(ScenarioData* data)
   :  GNE::Packet(ID),
   mScenarioData(data),
   mNumUnitPackets(0)
{
}

InitializingPacket::InitializingPacket()
: GNE::Packet(ID)
{
}

InitializingPacket::InitializingPacket( const InitializingPacket& p )
   :  GNE::Packet(ID),
   mScenarioData(p.mScenarioData),
   mNumUnitPackets(p.mNumUnitPackets)
{
}

/*InitializingPacket& InitializingPacket::operator=(const InitializingPacket& p) {
	mScenarioData = p.mScenarioData;
}*/

void InitializingPacket::writePacket( GNE::Buffer &raw ) const
{
   GNE::Packet::writePacket(raw);

   raw << mScenarioData->mapName;
   raw << mScenarioData->timeLimit;
   raw << mScenarioData->goalTime;
   raw << mScenarioData->forwardModel;
   raw << mScenarioData->numInstances;
   raw << (unsigned int)(mScenarioData->sides.size());
   for (unsigned int i = 0; i< mScenarioData->sides.size(); i++)
   {
   		raw << (int)mScenarioData->sides[i]->side;
   		raw << mScenarioData->sides[i]->total;
   		raw << (unsigned int)(mScenarioData->sides[i]->units.size());
   }
}

void InitializingPacket::readPacket( GNE::Buffer &raw )
{
   std::ostringstream oss;
   unsigned int sides_size, units_size;

   GNE::Packet::readPacket(raw);

   mScenarioData = new ScenarioData();
   raw >> mScenarioData->mapName;
   raw >> mScenarioData->timeLimit;
   raw >> mScenarioData->goalTime;
   int fm;
   raw >> fm;
   mScenarioData->forwardModel = (bool)fm;
   raw >> mScenarioData->numInstances;
   raw >> sides_size;
   for (unsigned int i = 0; i< sides_size; i++)
   {
   		ScenarioSideData *sideData = new ScenarioSideData();
   		int side;
   		raw >> side;
   		sideData->side = (UnitSide)side;
   		raw >> sideData->total;
   		raw >> units_size;
   		mNumUnitPackets += units_size;
   		mScenarioData->sides.push_back(sideData);
   }
}

int InitializingPacket::getSize() const
{
   int count = 0;

   count += Packet::getSize();
   count += GNE::Buffer::getSizeOf(mScenarioData->mapName);
   count += sizeof mScenarioData->timeLimit;
   count += sizeof mScenarioData->goalTime;
   count += sizeof mScenarioData->numInstances;
   count += sizeof mScenarioData->forwardModel;
   count += sizeof (unsigned int);
   for (unsigned int i = 0; i< mScenarioData->sides.size(); i++)
   {
   		count += sizeof (int);
   		count += sizeof mScenarioData->sides[i]->total;
   		count += sizeof (unsigned int);
   }
   //std::cout << " >> initPacketCount: " << count << std::endl;
   return count;
}

/////////// InitializingPacket ////////////////

const int InitializingUnitsPacket::ID = GNE::PacketParser::MIN_USER_ID + 3;

InitializingUnitsPacket::InitializingUnitsPacket(ScenarioUnitData* data, int side)
   :  GNE::Packet(ID),
   mScenarioUnitData(data),
   mSide(side)
{
}

InitializingUnitsPacket::InitializingUnitsPacket()
: GNE::Packet(ID)
{
}

InitializingUnitsPacket::InitializingUnitsPacket( const InitializingUnitsPacket& p )
   :  GNE::Packet(ID),
      mScenarioUnitData(p.mScenarioUnitData),
      mSide(p.mSide)
{
}

/*InitializingPacket& InitializingPacket::operator=(const InitializingPacket& p) {
	mScenarioData = p.mScenarioData;
}*/

void InitializingUnitsPacket::writePacket( GNE::Buffer &raw ) const
{
   GNE::Packet::writePacket(raw);

    raw << mSide;
	raw << (int)mScenarioUnitData->type;
	raw << mScenarioUnitData->instance;
	raw << mScenarioUnitData->number;
	raw << (unsigned int)(mScenarioUnitData->positions.size());
	for (unsigned int k = 0; k < mScenarioUnitData->positions.size(); k++ ) {
		raw << mScenarioUnitData->positions[k].x();
		raw << mScenarioUnitData->positions[k].y();
	}
}

void InitializingUnitsPacket::readPacket( GNE::Buffer &raw )
{
   std::ostringstream oss;
   unsigned int positions_size;

   GNE::Packet::readPacket(raw);

    raw >> mSide;
	ScenarioUnitData *unitData = new ScenarioUnitData();
	int type;
	raw >> type;
	unitData->type = (UnitType)type;
	raw >> unitData->instance;
	raw >> unitData->number;
	raw >> positions_size;
	for (unsigned int k = 0; k < positions_size; k++ ) {
		osg::Vec2 pos;
		raw >> pos.x();
		raw >> pos.y();
		unitData->positions.push_back(pos);
	}
	mScenarioUnitData = unitData;
}

int InitializingUnitsPacket::getSize() const
{
   int count = 0;

   count += Packet::getSize();
   count += sizeof mSide;
	count += sizeof (int);
	count += sizeof mScenarioUnitData->instance;
	count += sizeof mScenarioUnitData->number;
	count += sizeof (unsigned int);
	for (unsigned int k = 0; k < mScenarioUnitData->positions.size(); k++ ) {
		count += sizeof mScenarioUnitData->positions[k].x();
		count += sizeof mScenarioUnitData->positions[k].y();
	}

   return count;
}

/////////// UnitInitPacket ////////////////

const int UnitInitPacket::ID = GNE::PacketParser::MIN_USER_ID + 4;

UnitInitPacket::UnitInitPacket(const Unit* unit)
   :  GNE::Packet(ID)
{
	// set the ID to the unit name
	// (while it is unique, there is no problem with doing this)
	m_UniqueUnitID = unit->GetOSGNode()->getName();
	m_IsFlying = unit->getIsFlying();
	m_Side = unit->getSide();
	mClientID = unit->getClientID();
	mUnitID = unit->getUnitID();

	// copy the unit mesh names into a vector to transmit it in the packet
	std::map<dtCore::Transformable*, std::string>::const_iterator it;
	for (it = unit->GetNetworkingChildrenMeshes().begin();
		 	it != unit->GetNetworkingChildrenMeshes().end(); it++)
	 {
	 	m_ObjectModelNames.push_back(it->second);
	 }
}

UnitInitPacket::UnitInitPacket()
: GNE::Packet(ID)
{
}

UnitInitPacket::UnitInitPacket( const UnitInitPacket& p )
: GNE::Packet(ID)
{
	m_UniqueUnitID = p.m_UniqueUnitID;
	m_IsFlying = p.m_IsFlying;
	m_ObjectModelNames = p.m_ObjectModelNames;
	m_Side = p.m_Side;
	mClientID = p.mClientID;
	mUnitID = p.mUnitID;
}

int UnitInitPacket::getSize() const
{
    return Packet::getSize() + GNE::Buffer::getSizeOf(m_UniqueUnitID) +
   		  sizeof(int) + sizeof(m_Side) + sizeof(mClientID) + sizeof(mUnitID) + GetPacketSizeOf(m_ObjectModelNames);
}

void UnitInitPacket::writePacket( GNE::Buffer& raw ) const
{
   GNE::Packet::writePacket(raw);
   raw << m_UniqueUnitID;
   raw << (int)m_IsFlying;
   raw << m_Side;
   raw << mClientID;
   raw << mUnitID;
   raw << m_ObjectModelNames;
}

void UnitInitPacket::readPacket( GNE::Buffer& raw )
{
   int tmp;
   GNE::Packet::readPacket(raw);
   raw >> m_UniqueUnitID;
   raw >> tmp;
   m_IsFlying = (bool)tmp;
   raw >> m_Side;
   raw >> mClientID;
   raw >> mUnitID;
   raw >> m_ObjectModelNames;
}

/////////// EventPacket ////////////////

const int EventPacket::ID = GNE::PacketParser::MIN_USER_ID + 5;

EventPacket::EventPacket(Event::EventType event, std::vector<std::string>& participants,
					 	 std::vector<double>& data)
   :  GNE::Packet(ID)
{
	m_Event = event;
	m_Participants = participants;
	m_Data = data;
}

EventPacket::EventPacket()
: GNE::Packet(ID)
{
}

EventPacket::EventPacket( const EventPacket& p )
: GNE::Packet(ID)
{
	m_Event = p.m_Event;
	m_Participants = p.m_Participants;
	m_Data = p.m_Data;
}

int EventPacket::getSize() const
{
    return Packet::getSize() + sizeof(int) +
    	   GetPacketSizeOf(m_Participants) + GetPacketSizeOf(m_Data);
}

void EventPacket::writePacket( GNE::Buffer& raw ) const
{
   GNE::Packet::writePacket(raw);
   raw << (int)m_Event;
   raw << m_Participants;
   raw << m_Data;
}

void EventPacket::readPacket( GNE::Buffer& raw )
{
   int tmp;
   GNE::Packet::readPacket(raw);
   raw >> tmp;
   m_Event = (Event::EventType)tmp;
   raw >> m_Participants;
   raw >> m_Data;
}

void EventPacket::dumpPacket( std::ostringstream& ss ) const
{
	switch (m_Event)
	{
		case Event::POSITION:
			ss << "POS  ";
			break;
		case Event::HEADING:
			ss << "HEAD ";
			break;
		case Event::POSITION_AND_HEADING:
			ss << "POSH ";
			break;
		case Event::NEW_GOAL:
			ss << "GOAL ";
			break;
		case Event::SELECTED:
			ss << "SEL  ";
			break;
		case Event::DETONATION:
			ss << "DETN ";
			break;
		case Event::SEES:
			ss << "SEES ";
			break;
		case Event::HIDE:
			ss << "HIDE ";
			break;
		case Event::FIRE:
			ss << "FIRE ";
			break;
		case Event::HURT:
			ss << "HURT ";
			break;
		case Event::PAUSE:
			ss << "PAUSE ";
			break;
        case Event::GAME_OVER:
            ss << "GMOV ";
            break;
        case Event::TIME_SYNC:
        	ss << "SYNC ";
        	break;
        case Event::TIME_SCALE:
        	ss << "SCLE ";
        	break;
        default:
        	ss << "ERR  ";
        	break;
	}

   for (unsigned int i = 0; i<m_Participants.size(); i++)
   {
	    ss << m_Participants[i] << " ";
   }

   for (unsigned int i = 0; i<m_Data.size(); i++)
   {
	    ss << m_Data[i] << " ";
   }
   ss << std::endl;
}


/////////// IdPacket ////////////////

const int IdPacket::ID = GNE::PacketParser::MIN_USER_ID + 6;

IdPacket::IdPacket(int id)
   :  GNE::Packet(ID)
{
	m_Id = id;
}

IdPacket::IdPacket()
: GNE::Packet(ID)
{
}

IdPacket::IdPacket( const IdPacket& p )
: GNE::Packet(ID)
{
	m_Id = p.m_Id;
}

int IdPacket::getSize() const
{
    return Packet::getSize() + sizeof(m_Id);
}

void IdPacket::writePacket( GNE::Buffer& raw ) const
{
   GNE::Packet::writePacket(raw);
   raw << m_Id;
}

void IdPacket::readPacket( GNE::Buffer& raw )
{
   GNE::Packet::readPacket(raw);
   raw >> m_Id;
}

void IdPacket::dumpPacket( std::ostringstream& ss ) const
{
   ss << m_Id << std::endl;
}


/////////// UnitSavePacketRequest ////////////////

const int UnitSavePacketRequest::ID = GNE::PacketParser::MIN_USER_ID + 7;

UnitSavePacketRequest::UnitSavePacketRequest(int id)
   :  GNE::Packet(ID)
{
	m_Client = id;
}

UnitSavePacketRequest::UnitSavePacketRequest()
: GNE::Packet(ID)
{
}

UnitSavePacketRequest::UnitSavePacketRequest( const UnitSavePacketRequest& p )
: GNE::Packet(ID)
{
	m_Client = p.m_Client;
}

int UnitSavePacketRequest::getSize() const
{
    return Packet::getSize() + sizeof(m_Client);
}

void UnitSavePacketRequest::writePacket( GNE::Buffer& raw ) const
{
   GNE::Packet::writePacket(raw);
   raw << m_Client;
}

void UnitSavePacketRequest::readPacket( GNE::Buffer& raw )
{
   GNE::Packet::readPacket(raw);
   raw >> m_Client;
}

void UnitSavePacketRequest::dumpPacket( std::ostringstream& ss ) const
{
   ss << m_Client << std::endl;
}



/////////// UnitSavePacket ////////////////

const int UnitSavePacket::ID = GNE::PacketParser::MIN_USER_ID + 8;

UnitSavePacket::UnitSavePacket(Unit *unit, int num)
   :  GNE::Packet(ID)
{
	boost::archive::binary_oarchive oa(m_Oss);
	oa.register_type(static_cast<UnitTank*>(NULL));
	oa.register_type(static_cast<UnitSoldier*>(NULL));
	const Unit* const u = unit;
	oa << u;
	m_Id = u->getClientID();
	//total number of units we're sending
	m_Num = num;
}

UnitSavePacket::UnitSavePacket(std::string str, int num, int id) {
	m_Id = id;
	m_Num = num;
	m_Oss.str(str);
}

UnitSavePacket::UnitSavePacket()
: GNE::Packet(ID)
{
}

UnitSavePacket::UnitSavePacket( const UnitSavePacket& p )
: GNE::Packet(ID), osg::Referenced()
{
	m_Id = p.m_Id;
	m_Num = p.m_Num;
	m_Oss.str(p.m_Oss.str());
}

int UnitSavePacket::getSize() const
{
    return Packet::getSize() +
    sizeof(m_Id) +
    sizeof(m_Num) +
    sizeof(unsigned int) +
    m_Oss.str().length();
}

void UnitSavePacket::writePacket( GNE::Buffer& raw ) const
{
   GNE::Packet::writePacket(raw);
   raw << m_Id;
   raw << m_Num;
   //raw << m_Oss.str(); GNE doesnt allow for serialisation of strings of length > 255 :-(
   unsigned int len = m_Oss.str().length();
   if (len > 496) {
	   LOG_ERROR("Serialized unit must be less than 496 bytes to fit in a packet");
   }
   raw << len;
   // write the string data directly
   raw.writeRaw((const GNE::gbyte*)m_Oss.str().data(), len);
}

void UnitSavePacket::readPacket( GNE::Buffer& raw )
{
   GNE::Packet::readPacket(raw);
   raw >> m_Id;
   raw >> m_Num;
   //std::string str;
   //raw >> str; GNE doesnt allow for deserialisation of strings of length > 255 :-(
   //m_Oss.str(str);
   unsigned int len;
   raw >> len;
   int position = raw.getPosition();
   GNE::gbyte* data = raw.getData();
   //read the raw string data from the packet directly
   std::string str((char*)&data[position], len);
   raw.setPosition(position + len);
   m_Oss.str(str);
}

void UnitSavePacket::dumpPacket( std::ostringstream& ss ) const
{
   ss.str(m_Oss.str());
}

/////////// CommandPacket ////////////////

const int CommandPacket::ID = GNE::PacketParser::MIN_USER_ID + 9;

CommandPacket::CommandPacket(Command::CommandType command, osg::Vec3 pos, std::vector<int>& units, float duration, float timescale)
: GNE::Packet(ID), mCommand(command), mPos(pos), mUnits(units), mDuration(duration), mTimeScale(timescale)
{
}

CommandPacket::CommandPacket(Command::CommandType command, std::vector<int>& units, float duration, float timescale)
: GNE::Packet(ID), mCommand(command), mPos(osg::Vec3(0,0,0)), mUnits(units), mDuration(duration), mTimeScale(timescale)
{
}

CommandPacket::CommandPacket()
: GNE::Packet(ID), mCommand(Command::NONE), mPos(osg::Vec3(0,0,0)), mDuration(1.0), mTimeScale(1.0)
{
}

CommandPacket::CommandPacket( const CommandPacket& p )
: GNE::Packet(ID)
{
	mCommand = p.mCommand;
	mPos = p.mPos;
	mUnits = p.mUnits;
	mDuration = p.mDuration;
	mTimeScale = p.mTimeScale;
}

int CommandPacket::getSize() const
{
	return Packet::getSize() +
	sizeof(unsigned int) +
	sizeof(mDuration) +
	sizeof(mTimeScale) +
	sizeof(mPos[0]) +
	sizeof(mPos[1]) +
	sizeof(mPos[2]) +
	GetPacketSizeOf(mUnits);
}

void CommandPacket::writePacket( GNE::Buffer& raw ) const
{
   GNE::Packet::writePacket(raw);
   raw << (unsigned int)mCommand;
   raw << mDuration;
   raw << mTimeScale;
   raw << mPos[0];
   raw << mPos[1];
   raw << mPos[2];
   raw << mUnits;
}

void CommandPacket::readPacket( GNE::Buffer& raw )
{
	GNE::Packet::readPacket(raw);
	unsigned int command;
	raw >> command;
	mCommand = (Command::CommandType)command;
	raw >> mDuration;
	raw >> mTimeScale;
	raw >> mPos[0];
	raw >> mPos[1];
	raw >> mPos[2];
	raw >> mUnits;
}

void CommandPacket::dumpPacket( std::ostringstream& ss ) const
{
   ss << mCommand << std::endl;
}
