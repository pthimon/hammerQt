#ifndef MYNETWORK_H_
#define MYNETWORK_H_

//#include <QObject>
#include <dtNet/netmgr.h>
#include <dtCore/refptr.h>
#include <dtCore/object.h>

#include "packets.h"

class HypothesisClient;

struct SendPacketRequest {
	std::string address;
	GNE::Packet* packet;
};

/** Deriving from NetMgr will allow use to overwrite some virtual methods.
  * We'll use these methods for controlling our network connections.
  */
class MyNetwork : /*public QObject,*/ public dtNet::NetMgr
{
	//Q_OBJECT

public:
   MyNetwork( dtCore::Scene* scene );
   virtual ~MyNetwork() {}

   virtual void OnReceive( GNE::Connection &conn );
   virtual void OnExit( GNE::Connection &conn );
   virtual void OnDisconnect( GNE::Connection &conn );
   virtual void OnNewConn( GNE::SyncConnection &conn);
   virtual void OnConnect( GNE::SyncConnection &conn);
   virtual void OnFailure( GNE::Connection &conn, const GNE::Error &error );
   virtual void GetPositionOf(std::string ID, osg::Vec3& XYZ, osg::Vec3& HPR);
   ///Send an UNRELIABLE packet to the given address
   virtual void SendUnreliablePacket( const std::string &address, GNE::Packet &packet );
   virtual void SendPacketToClient( int client, GNE::Packet &packet);
   virtual void SendPacketToAllClients(GNE::Packet &packet);
   virtual void SendPacketToAllHypothesisClients(GNE::Packet &packet);
   virtual void SendEvent( EventPacket packet );

   /** Register an init packet for a unit which will is guaranteed
    * to be be sent to all clients.
    */
   virtual void RegisterInitPacket(UnitInitPacket packet);
   // TODO: add removing function

   void SetInitializingPacket(InitializingPacket & p) {m_InitPacket = p;}
   InitializingPacket GetInitializingPacket() { return m_InitPacket; }
   bool WaitForClient();
   bool WaitForClient(HypothesisClient* client);
   bool WaitForServer();

   void PreFrame( const double deltaFrameTime );

   GNE::Connection* GetConnection(int clientID);
   void DisconnectConnection(std::string address);

   std::set<int> GetClients();

   std::vector<std::string> getRemoteUnits(int client);
   double getCommandTimeRemaining() { return mCommandTimeRemaining; }

   void SendQueuedPacket(const std::string& addr, GNE::Packet* p);

private:
   dtCore::RefPtr< dtCore::Scene >  mScene;
   std::queue< dtCore::RefPtr<dtCore::Object> >    mObjectsToAdd;
   std::queue< std::string >        mIDsToRemove;

   std::queue<EventPacket> m_EventQueue;

   ///a map of player ID strings and their corresponding Object
   typedef std::map<std::string, dtCore::RefPtr<dtCore::Object> > StringObjectMap;
   StringObjectMap mOtherPlayerMap;

   void MakePlayer(const std::string& ownerID);
   GNE::Mutex m_Mutex;

   InitializingPacket m_InitPacket;

   bool m_InitPacketReceived;
   bool m_InitPacketSent;
   bool m_InitPacketStarted;
   unsigned int m_InitUnitPackets;
   bool m_IdPacketSent;
   bool m_NewWorldState;

   unsigned int m_UnitInitPacketsToSend;

   std::vector<GNE::Connection*> mClientConnections;
   std::vector<GNE::Connection*> mHypothesisClientConnections;

   std::vector<UnitInitPacket> m_UnitInitPackets;
   std::map<std::string, PositionPacket> m_PositionPacketStore;
   std::queue<EventPacket> m_NewEventPackets;
   std::queue<UnitInitPacket> m_NewInitPackets;
   std::map<int, GNE::Connection*> m_Clients;
   std::queue<UnitSavePacketRequest> m_RemoteUnitRequests;
   std::map<int, std::queue<UnitSavePacket> > m_SavedUnits;
   std::map<int, bool> m_Waiting;
   int m_ReceivingWorldState;
   std::vector<std::string> m_WorldState;
   std::queue<CommandPacket> mCommands;
   double mCommandTimeRemaining;
   bool mQuit;

   std::queue<std::pair<int, std::string> > m_ClientQueue;
   std::queue<UnitSavePacket> m_PredictionsQueue;
   std::queue<GNE::Connection*> mNewConnections;

   std::queue<SendPacketRequest> m_SendPacketQueue;
};

#endif
