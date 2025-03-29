#ifndef network_h
#define network_h

#include "buffer.hh"

#include <stdbool.h>
#include <stddef.h>

#define NET_ERROR_MESSAGE_LEN 128
#define SERVER_ACCEPT_QUEUE_LIMIT 4
#define NET_BUFFER_SIZE 1024

struct Socket {
    Buffer write_buf;

    char read_buf[NET_BUFFER_SIZE];
    int read_size;

    int fd;
    bool is_init;
};

bool InitNetwork(const char * log_name);
Socket CreateClient(const char * ip, const char * port);
Socket CreateServer(const char * port);
bool AcceptConnection(const Socket * server, Socket * out);
int NetWrite(const Socket * socket, void * data, int size);
bool NetWriteAll(const Socket * socket, void * data, int size);
int NetRead(const Socket * socket, void * buffer, int size);
bool NetReadAll(const Socket * socket, void * buffer, int size);
void CloseSocket(const Socket * socket);
const char * GetNetError(void);
void ShutdownNet(void);
void NetLog(const char * format, ...);

#endif /* network_h */
