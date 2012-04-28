#ifndef UNITREMOTE_H_
#define UNITREMOTE_H_

#include <map>
#include <vector>
#include <string>
#include <sstream>

#include <gnelib/gnetypes.h>
#include <gnelib/Packet.h>

#include "../common.h"
#include "Unit.h"
#include "../packets.h"

class UnitRemote : public Unit
{
public:
    UnitRemote(UnitInitPacket& init_packet, bool controlled = false, int client = -1, int id = -1 );
		
	virtual ~UnitRemote();
	
	virtual void update();
	virtual void setVisible(bool visible);
	
protected:
	virtual void LoadMeshes();	
	
private:
	std::vector<std::string> m_ObjectModelNames;
};

#endif /*UNITREMOTE_H_*/
