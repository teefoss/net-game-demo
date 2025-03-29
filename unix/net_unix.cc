#include "../net.hh"
#include "misc.hh"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

static FILE* log_file;

// TODO: use __thread or similar if we go multithreaded!
static char err_str[NET_ERROR_MESSAGE_LEN] = "No error";

static void set_err(const char * format, ...)
{
    va_list args;
    va_start(args, format);
    vsnprintf(err_str, NET_ERROR_MESSAGE_LEN, format, args);
    va_end(args);
}

// TODO: make error messages more generic, don't mention fcntl etc?

bool InitNetwork(const char * log_name)
{
    log_file = fopen(log_name, "w");

    if (log_file == nullptr) {
        set_err("Failed to open %s", log_name);
        return false;
    }

    return true;
}

// TODO: public SetBlocking(bool)
static bool SetNonBlocking(int socket)
{
    // Set socket to non blocking.
    int flags = fcntl(socket, F_GETFL);
    if ( flags == -1 ) {
        set_err("fcntl(F_GETFL) failed: %s", strerror(errno));
        return false;
    }

    flags |= O_NONBLOCK;
    if ( fcntl(socket, F_SETFL, flags) != 0 ) {
        set_err("fcntl(F_SETFL) failed: %s", strerror(errno));
        return false;
    }

    return true;
}

Socket CreateClient(const char * ip, const char * port)
{
    assert(port != nullptr);
    assert(ip != nullptr);

    Socket result = { 0 };
    BufferInit(&result.write_buf, 1024); // TODO: extract init code commont to both client and server

    struct addrinfo hints = {
        .ai_family = AF_UNSPEC, // don't care IPv4 or IPv6
        .ai_socktype = SOCK_STREAM // TCP stream sockets
    };

    struct addrinfo *server_info;  // will point to the results

    int rc = getaddrinfo(ip, port, &hints, &server_info);
    if ( rc != 0 ) {
        set_err("getaddrinfo error: %s", gai_strerror(rc));
        goto done;
    }

    // Create file descriptor
    result.fd = socket(server_info->ai_family,
                       server_info->ai_socktype,
                       (int)server_info->ai_protocol);

    if ( result.fd < 0 ) {
        set_err("socket() failed: %s\n", strerror(errno));
        goto done;
    }

    // TODO: use func
    // Set socket to non blocking.
    {
        int flags = fcntl(result.fd, F_GETFL);
        if ( flags == -1 ) {
            set_err("fcntl(F_GETFL) failed: %s", strerror(errno));
            goto done;
        }

        flags |= O_NONBLOCK;
        if ( fcntl(result.fd, F_SETFL, flags) != 0 ) {
            set_err("fcntl(F_SETFL) failed: %s", strerror(errno));
            goto done;
        }
    }

    if ( connect(result.fd, server_info->ai_addr, (int)server_info->ai_addrlen) == -1 ) {
        if ( errno == EINPROGRESS ) {

            fd_set set;
            FD_ZERO(&set);
            FD_SET(result.fd, &set);
            struct timeval timeout = { 300, 0 };

            int rc = select(result.fd + 1, nullptr, &set, nullptr, &timeout);
            if ( rc == -1 ) {
                set_err("select failed: %s\n", strerror(errno));
                goto done;
            } else if ( rc == 0 ) {
                set_err("client connection timed out");
                goto done;
            }
        } else {
            set_err("connect error: %s", strerror(errno));
            goto done;
        }
    }

    result.is_init = true;
done:
    if ( server_info ) {
        freeaddrinfo(server_info);
    }

    return result;
}

Socket CreateServer(const char * port)
{
    assert(port != nullptr);

    Socket result = { 0 };
    BufferInit(&result.write_buf, 1024);

    struct addrinfo hints = {
        .ai_family = AF_UNSPEC, // don't care IPv4 or IPv6
        .ai_socktype = SOCK_STREAM, // TCP stream sockets
        .ai_flags = AI_PASSIVE
    };

    struct addrinfo *server_info;  // will point to the results

    // See beej's network guide for more details.
    // Lookup network info for server type socket.
    int rc = getaddrinfo("0.0.0.0", port, &hints, &server_info);
    if (rc != 0) {
        set_err("getaddrinfo error: %s", gai_strerror(rc));
        goto done;
    }

    result.fd = socket(server_info->ai_family,
                       server_info->ai_socktype,
                       server_info->ai_protocol);

    if ( result.fd < 0 ) {
        set_err("socket() failed: %s", strerror(errno));
        goto error;
    }

    // Set socket to non blocking, so we can call accept() without blocking.
    {
        int server_socket_flags = fcntl(result.fd, F_GETFL);
        if (server_socket_flags == -1){
            set_err("fcntl(F_GETFL) failed: %s", strerror(errno));
            goto error;
        }

        server_socket_flags |= O_NONBLOCK;
        rc = fcntl(result.fd, F_SETFL, server_socket_flags);
        if (rc != 0){
            set_err("fcntl(F_SETFL) failed: %s", strerror(errno));
            goto error;
        }
    }

    // Bind to a specific port.
    rc = bind(result.fd,
              server_info->ai_addr,
              (int)server_info->ai_addrlen);
    if (rc != 0) {
        set_err("bind() failed: %s\n", strerror(errno));
        goto error;
    }

    // Listen on the socket for incoming connections.
    rc = listen(result.fd, SERVER_ACCEPT_QUEUE_LIMIT);
    if (rc != 0) {
        set_err("listen() failed: %s", strerror(errno));
        goto done;
    }

    result.is_init = true;

    goto done;
error:
    close(result.fd);
done:
    if ( server_info ) {
        freeaddrinfo(server_info);
    }

    return result;
}

bool AcceptConnection(const Socket * server, Socket * out)
{
    assert(server != nullptr);
    assert(!out->is_init);

    out->fd = accept(server->fd, nullptr, nullptr);
    out->is_init = false;

    if ( out->fd == -1 ) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            // accept didn't fail, but there was no connection:
            return true; // TODO: distinguish return of fail vs none available
        }

        // accept failed:
        set_err("accept() failed: %s", strerror(errno));
        return false;
    }

    BufferInit(&out->write_buf, 1024);
    out->is_init = true;
    return true;
}

int NetWrite(const Socket * socket, void * data, int size)
{
    assert(socket != nullptr);
    assert(data != nullptr);
    assert(size > 0);

    ssize_t size_sent = send(socket->fd, data, size, 0);

    if ( size_sent == -1 ) {
        if ( errno == EWOULDBLOCK || errno == EAGAIN ) {
            return 0;
        }

        set_err("Failed to send data: %s", strerror(errno));
        return -1;
    }

    return (int)size_sent;
}

// TODO: here and unix: move to net_common.c
bool NetWriteAll(const Socket * socket, void * data, int size)
{
    int bytes_sent = 0;
    int bytes_left = size;

    while ( bytes_sent < size ) {
        int n = NetWrite(socket, (char *)data + bytes_sent, bytes_left);

        if ( n == -1 ) {
            return false;
        }

        bytes_sent += n;
        bytes_left -= n;
    }

    return true;
}

int NetRead(const Socket * socket, void * buffer, int size)
{
    assert(socket != nullptr);
    assert(buffer != nullptr);
    assert(size > 0);

    ssize_t received = recv(socket->fd, buffer, size, 0);
    if ( received < 0 ) {
        if ( errno == EWOULDBLOCK || errno == EAGAIN ) {
            // Received nothing, but socket was non-blocking so it's okay.
            return 0;
        }

        set_err("Error receiving data: %s", strerror(errno));
        return -1;
    }

    return (int)received;
}

// TODO: here and unix: move to net_common.c
bool NetReadAll(const Socket * socket, void * buffer, int size)
{
    int bytes_read = 0;
    int bytes_left = size;

    while ( bytes_read < size ) {
        int n = NetRead(socket, buffer, bytes_left);
        if ( n == -1 ) {
            return false;
        }

        bytes_read += n;
        bytes_left -= n;
    }

    return true;
}

void CloseSocket(const Socket * socket)
{
    assert(socket != nullptr);
    close(socket->fd);
}

const char * GetNetError(void)
{
    return err_str;
}

void ShutdownNet(void)
{
    fclose(log_file);
}

void NetLog(const char * format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(log_file, format, args);
    fprintf(log_file, "\n");
    va_end(args);
}
