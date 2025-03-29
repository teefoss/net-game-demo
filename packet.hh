//
//  packet.hh
//  NetTest2
//
//  Created by Thomas Foster on 3/16/25.
//

#ifndef packet_hh
#define packet_hh

#include "misc.hh"
#include "net.hh"

typedef u16 PacketSize;

bool PacketRead(Socket * socket, Buffer * buffer);
bool PacketWrite(Socket * socket, Buffer * buffer);

#endif /* packet_hh */
