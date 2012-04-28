#include "common.h"

#include <osg/io_utils>
#include <dtUtil/stringutils.h>
#include <dtCore/scene.h>
#include <dtCore/system.h>
#include <dtCore/transform.h>

#include "mynetwork.h"
#include "packets.h"
#include "HTapp.h"
#include "units/UnitTank.h"
#include "units/UnitSoldier.h"
#include "models/InverseModelAttack.h"
#include "models/InverseModelRetreat.h"
#include "models/InverseModelDefend.h"
#include "models/InverseModelPath.h"
#include "models/InverseModelFormation.h"
#include "models/InverseModelPincer.h"
#include "models/InverseModelConvoy.h"
#include "Command.h"
#include "hypotheses/HypothesisClient.h"
#include "hypotheses/HypothesisManager.h"

using namespace dtUtil;
using namespace dtCore;

MyNetwork::MyNetwork( dtCore::Scene* scene )
   : mScene(scene),
   m_InitPacketReceived(false),
   m_InitPacketSent(false),
   m_InitPacketStarted(false),
   m_InitUnitPackets(0),
   m_IdPacketSent(false),
   m_NewWorldState(false),
   m_UnitInitPacketsToSend(0),
   m_ReceivingWorldState(0),
   mCommandTimeRemaining(std::numeric_limits<double>::max()),
   mQuit(false)
{
}

/**
 * Send an packet over UDP to a simulation client/server
 */
void MyNetwork::SendUnreliablePacket( const std::string &address, GNE::Packet &packet )
{
   mMutex.acquire();

   if (address != "all")
   {
	  GNE::Connection* conn = mConnections[address];
      if (conn)
    	  conn->stream().writePacket(packet, false );
      else
    	  LOG_ERROR("Trying to send unreliable packet to an unknown client")
   }
   else
   {
	  ConnectionIterator conns = mConnections.begin();
	  while (conns != mConnections.end())
	  {
		 //This fails if a connection is broken. We need some error checking here.
		 if ( conns->second->isConnected() && dtCore::System::GetInstance().IsRunning() )
		   {
			 (*conns).second->stream().writePacket(packet, false);
		   }
		 ++conns;
	  }
   }

   mMutex.release();
}

/**
 * Send packet on next call to MyNetwork::PreFrame
 */
void MyNetwork::SendQueuedPacket(const std::string& addr, GNE::Packet* p) {
	SendPacketRequest r;
	r.address = addr;
	r.packet = p;
	m_SendPacketQueue.push(r);
}

void MyNetwork::SendPacketToClient( int client, GNE::Packet &packet) {
	if (client >= 0) {
		GNE::Connection* conn = GetConnection(client);
		m_Mutex.acquire();
		if (conn != NULL) {
			conn->stream().writePacket(packet, true);
		} else {
			LOG_ERROR("Trying to send packet to an unknown client")
		}
		m_Mutex.release();
	} else {
		SendPacketToAllClients(packet);
	}
}

void MyNetwork::SendPacketToAllClients(GNE::Packet &packet) {
	m_Mutex.acquire();

	std::vector<GNE::Connection*>::iterator conns;
	for (conns = mClientConnections.begin(); conns != mClientConnections.end(); conns++) {
		(*conns)->stream().writePacket(packet, true);
	}

	m_Mutex.release();
}

void MyNetwork::SendPacketToAllHypothesisClients(GNE::Packet &packet) {
	m_Mutex.acquire();

	std::vector<GNE::Connection*>::iterator conns;
	for (conns = mHypothesisClientConnections.begin(); conns != mHypothesisClientConnections.end(); conns++) {
		(*conns)->stream().writePacket(packet, true);
	}

	m_Mutex.release();
}

///One or more GNE::Packets was received, let's do something with them
void MyNetwork::OnReceive( GNE::Connection &conn )
{
	m_Mutex.acquire();

	GNE::Packet* next = conn.stream().getNextPacket();

	while (next != 0) {
		int type = next->getType();

		// std::cout << "type: " << type << std::endl;

		if (type == GNE::PingPacket::ID) {
			//it's a PingPacket so lets process it.
			/*GNE::PingPacket& ping = *static_cast<GNE::PingPacket*> (next);
			if (ping.isRequest()) {
				ping.makeReply();
				conn.stream().writePacket(ping, true);
			} else {
				LOG_INFO("Ping: " + ping.getPingInformation().pingTime.toString());
			}*/
		} else if (type == IdPacket::ID) {
			IdPacket* packet = static_cast<IdPacket*> (next);
			dtUtil::Log::GetInstance().LogMessage(dtUtil::Log::LOG_INFO,
					__FUNCTION__,
					__LINE__,"Registered new connection with ID %d",
					packet->m_Id);
			if (conn.getLocalAddress(true).getPort() == HypothesisManager::getPort()) {
				LOG_DEBUG("Got Hypothesis ID Packet")
				//new hypothesis client
				m_ClientQueue.push(std::pair<int, std::string>(packet->m_Id, conn.getRemoteAddress(true).toString()));
			} else {
				//new simulation client
				m_Clients[packet->m_Id] = &conn;
			}
		} else if (type == UnitInitPacket::ID) {
			// store packet in a queue to add them later in PreFrame() as a new unit
			UnitInitPacket* packet = static_cast<UnitInitPacket*> (next);
			m_NewInitPackets.push(*packet);
		} else if (type == EventPacket::ID) {
			EventPacket* packet = static_cast<EventPacket*> (next);
			m_NewEventPackets.push(*packet);
		} else if (type == PositionPacket::ID) {
			// store the position packet in the position packet store
			// later it will be accessed by the appropriate unit
			PositionPacket *pos = static_cast<PositionPacket*> (next);
			m_PositionPacketStore[pos->mOwnerID] = *pos;

			if (GetIsServer()) {
				//rebroadcast the packet to all connections except for the
				//one who sent the packet in the first place
				ConnectionIterator conns = mConnections.begin();
				while (conns != mConnections.end()) {
					if (conns->first != conn.getRemoteAddress(true).toString()) {
						SendPacket(conns->first, *next);
					}
					++conns;
				}
			}
		} else if (type == PlayerQuitPacket::ID) {
			PlayerQuitPacket* playerQuitPacket =
					static_cast<PlayerQuitPacket*> (next);
			mIDsToRemove.push(playerQuitPacket->mPlayerID);
		} else if (type == InitializingPacket::ID && m_InitPacketReceived
				== false) {
			m_InitPacket = *(static_cast<InitializingPacket*> (next));
			m_InitPacketStarted = true;
		} else if (type == InitializingUnitsPacket::ID && m_InitPacketStarted == true && m_InitPacketReceived == false) {
			InitializingUnitsPacket p = *(static_cast<InitializingUnitsPacket*> (next));
			m_InitPacket.mScenarioData->sides[p.mSide]->units.push_back(p.mScenarioUnitData);
			m_InitUnitPackets++;
			if (m_InitPacket.mNumUnitPackets) {
				m_InitPacketReceived = true;
			}
		} else if (type == UnitSavePacketRequest::ID) {
			//TODO: is this thread-safe? move to preframe?
			theApp->sendLocalUnits();
		} else if (type == UnitSavePacket::ID) {
			UnitSavePacket p = *(static_cast<UnitSavePacket*> (next));
			if (GetIsServer()) {
				if (conn.getLocalAddress(true).getPort() == HypothesisManager::getPort()) {
					//got prediction from a hypothesis client
					//LOG_DEBUG("Got prediction from hypothesis client")
					m_PredictionsQueue.push(p);
				} else {
					//server saves units from client
					//LOG_INFO("Received unit from client");
					//stick the packets into a packet store to save
					m_SavedUnits[p.m_Id].push(p);
				}
			} else {
				//running as a client or a hypothesis client
				if (!m_ReceivingWorldState) {
					//LOG_INFO("Starting receiving units...")
					m_NewWorldState = true;
					//remove existing packets
					m_WorldState.clear();
				}
				//LOG_INFO("Got a unit");
				//save packet
				m_WorldState.push_back(p.m_Oss.str());
				m_ReceivingWorldState++;
				if (m_ReceivingWorldState == p.m_Num) {
					//LOG_INFO("All units received...");
					m_ReceivingWorldState = -1;
				}
			}
		} else if (type == CommandPacket::ID) {
			LOG_INFO("Received a command packet");
			CommandPacket p = *(static_cast<CommandPacket*> (next));
			mCommands.push(p);
		}

		delete next;
		next = conn.stream().getNextPacket();
	}

	m_Mutex.release();
}

///Create a new player to represent the remote guy
void MyNetwork::MakePlayer( const std::string& ownerID )
{
   m_Mutex.acquire();

   LOG_INFO("Making a new remote player named: " + ownerID)

   dtCore::RefPtr<dtCore::Object> object = new dtCore::Object(ownerID);
   object->LoadFile("models/uh-1n.ive");

   Transform transform( 0.0f, 0.0f, 5.0f );
   object->SetTransform( transform );

   //Insert the new Objects into our map of IDs->Objects
   StringObjectMap::value_type value( ownerID,  object );
   mOtherPlayerMap.insert( value );

   mObjectsToAdd.push( object );

   m_Mutex.release();
}

void MyNetwork::OnExit( GNE::Connection &conn )
{
   //do the default NetMgr behavior too
   NetMgr::OnExit(conn);

   if (!GetIsServer()) {
	   LOG_ERROR("Server has closed the connection, quitting...")
	   mQuit = true;
   }
}

void MyNetwork::OnDisconnect( GNE::Connection &conn )
{
   //RemoveConnection(&conn);
	LOG_DEBUG("Disconnected.");
	if (theApp->isForwardModel()) {
	   dtCore::System::GetInstance().Stop();
   }
}

/**
 * Server receives a new connection from a client
 * @BUG: The simulation clients must connect before the hypothesis clients
 */
void MyNetwork::OnNewConn( GNE::SyncConnection &conn)
{
	NetMgr::OnNewConn(conn);

	m_Mutex.acquire();
	// Save the connection
	if (conn.getConnection()->getLocalAddress(true).getPort() == theApp->getNetworkPort()) {
		// This is a simulation client connection
		mClientConnections.push_back(conn.getConnection().get());
	} else if (conn.getConnection()->getLocalAddress(true).getPort() == HypothesisManager::getPort()) {
		// This is a hypothesis client connection
		mHypothesisClientConnections.push_back(conn.getConnection().get());
		// tell the client that it is a hypothesis client (forward model)
		m_InitPacket.mScenarioData->forwardModel = true;
	} else {
		LOG_ERROR("Received a connection on an unrecognised port")
	}

	//send the initialising packet in preframe
	mNewConnections.push(conn.getConnection().get());

	m_Mutex.release();
}

void MyNetwork::OnConnect( GNE::SyncConnection &conn)
{
	// do what the parent does
	NetMgr::OnConnect(conn);

	m_Mutex.acquire();
	if (conn.getConnection()->getRemoteAddress(true).getPort() == theApp->getNetworkPort()) {
		mClientConnections.push_back(conn.getConnection().get());
	} else if (conn.getConnection()->getRemoteAddress(true).getPort() == HypothesisManager::getPort()) {
		mHypothesisClientConnections.push_back(conn.getConnection().get());
		mClientConnections.push_back(conn.getConnection().get());
	} else {
		LOG_ERROR("Made a connection to an unrecognised port")
	}
	m_Mutex.release();
}

void MyNetwork::PreFrame( const double deltaFrameTime )
{
	m_Mutex.acquire();

	if (mQuit) {
		theApp->Quit();
		return;
	}

	while( !mNewConnections.empty() ) {
		// send the initializing packet
		LOG_INFO("New Connection: Sending init packet");
		std::string addr = mNewConnections.front()->getRemoteAddress(true).toString();
		//send init packet
		SendPacket(addr, m_InitPacket);
		for (unsigned int i = 0; i< m_InitPacket.mScenarioData->sides.size(); i++)
	    {
			for (unsigned int j = 0; j < m_InitPacket.mScenarioData->sides[i]->units.size(); j++ )
			{
				InitializingUnitsPacket p(m_InitPacket.mScenarioData->sides[i]->units[j], i);
				SendPacket(addr, p);
			}
	    }
		m_InitPacketSent = true;

		// send all of the currently registered unit initialization packets
		for (unsigned int i = 0; i<m_UnitInitPackets.size(); i++) {
			SendPacket(mNewConnections.front()->getRemoteAddress(true).toString(), m_UnitInitPackets[i]);
		}

		LOG_INFO("New Connection: Complete");

		mNewConnections.pop();
	}

	//if we've finished simulating then send the world state
	if (theApp->isForwardModel()) {
		if (mCommandTimeRemaining != std::numeric_limits<double>::max()) {
			mCommandTimeRemaining -= theApp->getFrameTime();
		}
		if (mCommandTimeRemaining <= 0) {
			mCommandTimeRemaining = std::numeric_limits<double>::max();
			LOG_INFO("Sending predicted world state")
   			//send predicted world state to controller
   			theApp->sendWorldState(-1);
   			theApp->clearScene();
   		}
	}

	//if we're at the start of receiving a new world state then clear the scene
   	if (m_NewWorldState) {
   		//clear scene
   		theApp->clearScene();
   		m_NewWorldState = false;
   	}

	if (!theApp->isForwardModel()) {
		// send all new registered units to all existing connections
		// for some unknown reason to me, it can be sent only from here...
		for (unsigned int i = 0; i<m_UnitInitPacketsToSend; i++)
		{
			SendPacketToAllClients(m_UnitInitPackets[m_UnitInitPackets.size() - m_UnitInitPacketsToSend + i]);
		}
		m_UnitInitPacketsToSend = 0;

		// send all the events
		while ( !m_EventQueue.empty() )
		{
			SendPacketToAllClients(m_EventQueue.front());
			m_EventQueue.pop();
		}

		// create objects for incoming unit registrations
	   	while( !m_NewInitPackets.empty() )
	   	{
	   		theApp->CreateRemoteUnit(m_NewInitPackets.front());
			m_NewInitPackets.pop();
	   	}

	   	// send Remote Unit Request packets
	   	while( !m_RemoteUnitRequests.empty() ) {
	   		UnitSavePacketRequest p = m_RemoteUnitRequests.front();
	   		SendPacket(m_Clients[p.m_Client]->getRemoteAddress(true).toString(), p);
	   		m_RemoteUnitRequests.pop();
		}
	   	while ( !m_NewEventPackets.empty() )
	   	  {
	               theApp->ReceiveEvent(m_NewEventPackets.front().m_Event,
	                   m_NewEventPackets.front().m_Participants,
	                   m_NewEventPackets.front().m_Data);
                       m_NewEventPackets.pop();
	   	  }
	}

   	// send ID packet after connection is established
   	if (!m_IdPacketSent) {
		//send ID to server
		IdPacket p(theApp->getInstanceID());
		SendPacket("all", p);
		LOG_INFO("Sent ID packet")
		m_IdPacketSent = true;
   	}

   	// received all units
   	if (m_ReceivingWorldState == -1) {
   		LOG_INFO("Loading world state.")
   		//load received units
   		theApp->loadWorldState(&m_WorldState);
   		m_ReceivingWorldState = 0;
	}

   	while (!mCommands.empty()) {
   		//LOG_INFO("Got a command")
   		CommandPacket p = mCommands.front();
   		mCommands.pop();
   		std::vector<Unit*> units = theApp->getUnitsByIds(p.mUnits);

   		//set up timing
   		// \bug if more than one command, the most recent one's timing will be used
   		theApp->setTimeScale(p.mTimeScale);
   		mCommandTimeRemaining = p.mDuration;

   		//initate command
   		if (p.mCommand & Command::NONE) {
   			continue;
   		}
   		if (p.mCommand & Command::MANOEUVRES) {
			switch (p.mCommand & Command::MANOEUVRES)
			{
		  	case Command::MANOEUVRE_ATTACK:
		  		theApp->addInverseModel(new InverseModelAttack(units));
		  		break;
		  	case Command::MANOEUVRE_RETREAT:
				theApp->addInverseModel(new InverseModelRetreat(units));
				break;
		  	case Command::MANOEUVRE_DEFEND:
		  		theApp->addInverseModel(new InverseModelDefend(units));
		  		break;
		  	case Command::MANOEUVRE_PINCER:
		  		theApp->addInverseModel(new InverseModelPincer(units));
		  		break;
		  	case Command::MANOEUVRE_CONVOY:
				theApp->addInverseModel(new InverseModelConvoy(units));
				break;
		  	default:
		  		LOG_WARNING("Unknown manoeuvre command specified")
			}
		}
   		if (p.mCommand & Command::RENDEZ_VOUS) {
   			switch (p.mCommand & Command::RENDEZ_VOUS)
   			{
   			case Command::GOTO:
   				//position is an absolute
   				for (unsigned int i=0; i < units.size(); i++) {
	   				if (units[i]) units[i]->setGoalPosition(p.mPos);
	   			}
   				break;
   			case Command::HEADTO:
   				//position is an offset
   				for (unsigned int i=0; i < units.size(); i++) {
	   				if (units[i]) units[i]->setGoalPosition(units[i]->getPosition() + p.mPos);
	   			}
   				break;
   			case Command::PATHTO:
   				for (unsigned int i=0; i < units.size(); i++) {
   					InverseModelPath::createPath(units[i], p.mPos);
   				}
   				break;
   			default:
   				LOG_WARNING("Unknown rendez-vous command specified")
   			}
   		}
   		if (p.mCommand & Command::FORMATIONS) {
   			//convert the command into the formation enum by shifiting right by the bit position of the MSB of the rendez-vous bit mask + 1
   			int f = ((p.mCommand & Command::FORMATIONS) >> (int)(log(Command::RENDEZ_VOUS+1)/log(2)));
   			bool autoLeader = p.mPos[0];
   			bool autoArrange = p.mPos[1];
   			theApp->addInverseModel(new InverseModelFormation(units, (InverseModelFormation::UnitFormation)f, autoLeader, autoArrange));
   		}
   	}

   	//initialise new hypothesis clients with the remote address
   	while (!m_ClientQueue.empty()) {
   		HypothesisClient* c = HypothesisClient::getClient(m_ClientQueue.front().first);
   		if (c != NULL) {
   			c->setAddress(m_ClientQueue.front().second);
   		}
   		m_ClientQueue.pop();
   	}

   	//forward the predicted units positions to the hypothesis manager
   	while (!m_PredictionsQueue.empty()) {
		UnitSavePacket p = static_cast<UnitSavePacket>(m_PredictionsQueue.front());
		Unit *unit = theApp->deserialiseUnit(p.m_Oss.str(), true);
   		HypothesisManager* m = HypothesisManager::getManager();
   		if (m != NULL) {
   			m->newPrediction(p.m_Id, unit, p.m_Num);
   		}
   		m_PredictionsQueue.pop();
   	}

	//send all packets queued since last preframe
	while (!m_SendPacketQueue.empty()) {
		SendPacketRequest p = m_SendPacketQueue.front();
		SendPacket(p.address, *(p.packet));
		m_SendPacketQueue.pop();
		delete p.packet;
	}

	m_Mutex.release();
}

/**
 * Waits for one client (the init packet flag is set when a connection is received)
 */
bool MyNetwork::WaitForClient()
{
	m_Mutex.acquire();
	// wait for connection (do not waste CPU)
	while  (!m_InitPacketSent)
	{
		m_Mutex.release();
		sleep(1);
		m_Mutex.acquire();
	}
	m_Mutex.release();
	return true;
}

bool MyNetwork::WaitForClient(HypothesisClient* client)
{
	m_Mutex.acquire();
	// check that this is a hypothesis client id
	if (client == NULL) return false;
	// wait for connection (do not waste CPU)
	int timeout = 0;
	while  (!client->isInitialised() )
	{
		m_Mutex.release();
		sleep(1);
		std::cout << ".";
		std::flush(std::cout);
		PreFrame(1);
		m_Mutex.acquire();
		if (++timeout == 30) {
			return false; //timeout
		}
	}
	m_Mutex.release();
	return true;
}

bool MyNetwork::WaitForServer()
{
	// wait for connection (do not waste CPU)
//	while  (!m_InitPacketReceived) { sleep(1); }
	m_Mutex.acquire();
	int timeout = 0;
	// wait for connection (do not waste CPU)
	while  (!m_InitPacketReceived)
	{
		m_Mutex.release();
		sleep(1);
		if (++timeout == 30 || mQuit) return false;
		m_Mutex.acquire();
	}
	m_Mutex.release();
	return true;
}

void MyNetwork::RegisterInitPacket(UnitInitPacket packet)
{
    m_Mutex.acquire();

	m_UnitInitPackets.push_back(packet);
	m_UnitInitPacketsToSend++;

    m_Mutex.release();
}

void MyNetwork::OnFailure( GNE::Connection &/*conn*/, const GNE::Error &error )
{
   LOG_DEBUG(error.toString());
   if (theApp->isForwardModel()) {
	   dtCore::System::GetInstance().Stop();
   }
}

void MyNetwork::GetPositionOf(std::string ID, osg::Vec3& XYZ, osg::Vec3& HPR)
{
	m_Mutex.acquire();
	XYZ = m_PositionPacketStore[ID].mXYZ;
	HPR = m_PositionPacketStore[ID].mHPR;
	m_Mutex.release();
}

void MyNetwork::SendEvent( EventPacket packet )
{
	// send pause packet immediately, otherwise they won't get into the queue
	if (packet.m_Event == Event::PAUSE)
	{
		SendPacketToAllClients(packet);
	}
	else m_EventQueue.push(packet);
}

GNE::Connection* MyNetwork::GetConnection(int clientID) {
	m_Mutex.acquire();
	GNE::Connection* c = m_Clients[clientID];
	m_Mutex.release();
	return c;
}

std::set<int> MyNetwork::GetClients() {
	std::map<int,GNE::Connection*>::iterator conns;
	std::set<int> clients;

	m_Mutex.acquire();
	for (conns = m_Clients.begin(); conns != m_Clients.end(); conns++) {
		//add the simulation clients
		if (conns->second) clients.insert(conns->first);
	}
	m_Mutex.release();
	return clients;
}

std::vector<std::string> MyNetwork::getRemoteUnits(int client) {
	//send request
	//m_RemoteUnitRequests.push(UnitSavePacketRequest(client));
	std::cout << "Sending save packet request to client " << client << std::endl;
	UnitSavePacketRequest p(client);
	m_Mutex.acquire();
	SendPacket(m_Clients[client]->getRemoteAddress(true).toString(), p);

	//wait for responses
	std::vector<std::string> packets;
	int i = 1;
	m_Waiting[client] = true;
	while (m_Waiting[client]) {
		if (m_SavedUnits[client].empty()) {
			m_Mutex.release();
			sleep(1);
			m_Mutex.acquire();
		} else {
			UnitSavePacket p = m_SavedUnits[client].front();
			packets.push_back(p.m_Oss.str());
			m_SavedUnits[client].pop();
			if (p.m_Num == i) {
				m_Waiting[client] = false;
			} else {
				i++;
			}
		}
	}
	m_Mutex.release();

	return packets;
}

void MyNetwork::DisconnectConnection(std::string address) {
	mMutex.acquire();

	if (address != "all") {
		mConnections[address]->disconnect();
	} else {
		ConnectionIterator conns = mConnections.begin();
		while (conns != mConnections.end()) {
			// This fails if a connection is broken. We need some error checking here.
			(*conns).second->disconnect();
			++conns;
		}
	}
	mMutex.release();
}
