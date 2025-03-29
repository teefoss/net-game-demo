//
//  packet.cc
//  NetTest2
//
//  Created by Thomas Foster on 3/16/25.
//

#include "packet.hh"
#include "net.hh"
#include <string.h>
#include <stdio.h>

bool PacketRead(Socket * socket, Buffer * buffer)
{
    char * read_buf = socket->read_buf;

    // Read as much as the socket buffer allows.
    int bytes_read = 0;

    if ( NET_BUFFER_SIZE - socket->read_size > 0 ) {
        bytes_read = NetRead(socket,
                             read_buf + socket->read_size,
                             NET_BUFFER_SIZE - socket->read_size);
    }

    if ( bytes_read == -1 ) {
        fprintf(stderr, "Packet read: NetRead failed\n");
        return false;
    }

    socket->read_size += bytes_read;

    // Do we have an entire header?
    if ( socket->read_size < sizeof(PacketSize) ) {
        return false; // Nope.
    }

    PacketSize packet_size;
    memcpy(&packet_size, read_buf, sizeof(packet_size));

    // Do we have the entire packet after the size?
    size_t total_size = sizeof(PacketSize) + packet_size;
    if ( socket->read_size < total_size ) {
        return false; // Nope.
    }

    // We have an entire packet. Rejoice.

    BufferWrite(buffer, read_buf + sizeof(PacketSize), packet_size);

    // Move anything after this packet to the front of the buffer.
    memmove(read_buf, read_buf + total_size, socket->read_size - total_size);
    socket->read_size -= total_size;

    return true;
}

bool PacketWrite(Socket * socket, Buffer * buffer)
{
    PacketSize size = buffer->size;

    BufferWrite(&socket->write_buf, &size, sizeof(size)); // Write size
    BufferWrite(&socket->write_buf, buffer->data, size); // Write payload

    // Send buffer contents.
    if ( !NetWriteAll(socket, socket->write_buf.data, (int)socket->write_buf.size) ) {
        return false;
    }

    BufferClear(&socket->write_buf);

    return true;
}
