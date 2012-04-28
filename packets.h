#ifndef PACKETS_H_
#define PACKETS_H_

#include <osg/Vec3>
#include <gnelib/gnetypes.h>
#include <gnelib/Packet.h>
#include <gnelib/Buffer.h>
#include <vector>
#include <sstream>
#include <boost/archive/binary_oarchive.hpp>

#include "units/Unit.h"
#include "event.h"
#include "ScenarioData.h"
#include "models/InverseModelFormation.h"
#include "Command.h"

/** This custom packet will allow us to pass a position and rotation
 *  through the network connection.
 *  Two important things to remember:
 *  1) register the packet with GNE using
 *     GNE::PacketParser::defaultRegisterPacket<PositionPacket>();
 *  2) Every custom packet needs a unique ID
 */
class PositionPacket : public GNE::Packet
{
public:

   PositionPacket( osg::Vec3 xyz, osg::Vec3 hpr, const std::string &ownerID );

   ///default constructor
   PositionPacket();

   ///copy constructor
   PositionPacket( const PositionPacket &p );

   virtual ~PositionPacket() {}

   static const int ID;

   virtual int getSize() const;

   virtual void writePacket( GNE::Buffer &raw ) const;

   virtual void readPacket( GNE::Buffer &raw );

   osg::Vec3 mXYZ;
   osg::Vec3 mHPR;
   std::string mOwnerID;
};

/** This custom packet will allow us to pass a player's UniqueID upon
*  exit to let server and other clients known a player has quit the game.
*/
class PlayerQuitPacket : public GNE::Packet
{
public:

   PlayerQuitPacket( const std::string& playerID );
   PlayerQuitPacket();

public:

   PlayerQuitPacket( const PlayerQuitPacket& p );

   virtual ~PlayerQuitPacket() {}

   static const int ID;

   virtual int getSize() const;

   virtual void writePacket( GNE::Buffer& raw ) const;

   virtual void readPacket( GNE::Buffer& raw );

   std::string mPlayerID;
};

/** This custom packet will allow us to pass a player's UniqueID upon
*  exit to let server and other clients known a player has quit the game.
*/
class InitializingPacket : public GNE::Packet
{
public:
   InitializingPacket();

   InitializingPacket(ScenarioData* data);

   InitializingPacket( const InitializingPacket& p );

   virtual ~InitializingPacket() {}

   static const int ID;

   virtual int getSize() const;

   virtual void writePacket( GNE::Buffer& raw ) const;

   virtual void readPacket( GNE::Buffer& raw );

   ScenarioData* mScenarioData;
   unsigned int mNumUnitPackets;
};

/** This packet is for each unit entry in the scenario data
*/
class InitializingUnitsPacket : public GNE::Packet
{
public:
	InitializingUnitsPacket();

	InitializingUnitsPacket(ScenarioUnitData* data, int side);

	InitializingUnitsPacket( const InitializingUnitsPacket& p );

   virtual ~InitializingUnitsPacket() {}

   static const int ID;

   virtual int getSize() const;

   virtual void writePacket( GNE::Buffer& raw ) const;

   virtual void readPacket( GNE::Buffer& raw );

   ScenarioUnitData* mScenarioUnitData;
   unsigned int mSide;
};

/** this packet will create a new UnitRemote if sent.
 */
class UnitInitPacket : public GNE::Packet
{
public:
   UnitInitPacket();

   UnitInitPacket(const Unit* unit);

   UnitInitPacket( const UnitInitPacket& p );

   virtual ~UnitInitPacket() {}

   static const int ID;

   virtual int getSize() const;

   virtual void writePacket( GNE::Buffer& raw ) const;

   virtual void readPacket( GNE::Buffer& raw );

   std::string m_UniqueUnitID;
   std::vector<std::string> m_ObjectModelNames;
   bool m_IsFlying;
   int m_Side;
   int mClientID;
   int mUnitID;
};

/** this packet transmits system-wide events.
 */
class EventPacket : public GNE::Packet
{
public:
   //enum EventType {SEE, FIRE, HURT, GOAL, SELECT, PROJECTILE, PAUSE};

   EventPacket();

   EventPacket(Event::EventType event, std::vector<std::string>& participants, std::vector<double>& data);

   EventPacket( const EventPacket& p );

   virtual ~EventPacket() {}

   static const int ID;

   virtual int getSize() const;

   virtual void writePacket( GNE::Buffer& raw ) const;

   virtual void readPacket( GNE::Buffer& raw );

   // dump the packet contents into a stream (used for logging)
   virtual void dumpPacket( std::ostringstream& ss ) const;

   Event::EventType m_Event;
   std::vector<std::string> m_Participants;
   std::vector<double> m_Data;
};

/** this packet identifies the client.
 */
class IdPacket : public GNE::Packet
{
public:
	IdPacket();

	IdPacket(int id);

	IdPacket( const IdPacket& p );

   virtual ~IdPacket() {}

   static const int ID;

   virtual int getSize() const;

   virtual void writePacket( GNE::Buffer& raw ) const;

   virtual void readPacket( GNE::Buffer& raw );

   // dump the packet contents into a stream (used for logging)
   virtual void dumpPacket( std::ostringstream& ss ) const;

   int m_Id;
};

/** this packet requests remote units.
 */
class UnitSavePacketRequest : public GNE::Packet
{
public:
	UnitSavePacketRequest();

	UnitSavePacketRequest(int id);

	UnitSavePacketRequest( const UnitSavePacketRequest& p );

   virtual ~UnitSavePacketRequest() {}

   static const int ID;

   virtual int getSize() const;

   virtual void writePacket( GNE::Buffer& raw ) const;

   virtual void readPacket( GNE::Buffer& raw );

   // dump the packet contents into a stream (used for logging)
   virtual void dumpPacket( std::ostringstream& ss ) const;

   int m_Client;
};


/** this packet contains the complete state of a unit.
 */
class UnitSavePacket : public GNE::Packet, public osg::Referenced
{
public:
	UnitSavePacket();

	UnitSavePacket(Unit *u, int num);

	UnitSavePacket(std::string str, int num, int id);

	UnitSavePacket( const UnitSavePacket& p );

   virtual ~UnitSavePacket() {}

   static const int ID;

   virtual int getSize() const;

   virtual void writePacket( GNE::Buffer& raw ) const;

   virtual void readPacket( GNE::Buffer& raw );

   // dump the packet contents into a stream (used for logging)
   virtual void dumpPacket( std::ostringstream& ss ) const;

   std::ostringstream m_Oss;
   int m_Id;
   int m_Num;
};

/** this packet contains a position/formation command
 */
class CommandPacket : public GNE::Packet
{
public:
	CommandPacket(Command::CommandType command, osg::Vec3 pos, std::vector<int>& units, float duration = 1.0, float timescale = 1.0);
	CommandPacket(Command::CommandType command, std::vector<int>& units, float duration = 1.0, float timescale = 1.0);
	CommandPacket();

	CommandPacket( const CommandPacket& p );

   virtual ~CommandPacket() {}

   static const int ID;

   virtual int getSize() const;

   virtual void writePacket( GNE::Buffer& raw ) const;

   virtual void readPacket( GNE::Buffer& raw );

   // dump the packet contents into a stream (used for logging)
   virtual void dumpPacket( std::ostringstream& ss ) const;

   Command::CommandType mCommand;
   osg::Vec3 mPos;
   std::vector<int> mUnits;
   float mDuration;
   float mTimeScale;
};

///// template helper functions //////

template <class T> GNE::Buffer& operator<<(GNE::Buffer& raw, const std::vector<T>& out)
{
	raw << (unsigned int)(out.size());
	for (unsigned int i = 0; i<out.size(); i++)
	{
		raw << out[i];
	}

	return raw;
}

template <class T> GNE::Buffer& operator>>(GNE::Buffer& raw, std::vector<T>& in)
{
	unsigned int size;

	raw >> size;

	in.resize(size);

	for (unsigned int i = 0; i<in.size(); i++)
	{
		raw >> in[i];
	}

	return raw;
}

template <class T> unsigned int GetPacketSizeOf(const std::vector<T>& in)
{
	unsigned int size = 0;

	size += sizeof (unsigned int);
	for (unsigned int i = 0; i<in.size(); i++)
	{
		size += GNE::Buffer::getSizeOf(in[i]);
	}
	return size;
}

#endif //PACKETS_H_
