#include "net.hh"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

//#include <ws2tcpip.h>
//#include <winsock2.h>

static FILE * log_file;

// TODO: use __thread or similar if we go multithreaded!
static char err_str[NET_ERROR_MESSAGE_LEN] = "No error";
static char windows_error_str[NET_ERROR_MESSAGE_LEN];

static void set_err(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vsnprintf(err_str, NET_ERROR_MESSAGE_LEN, format, args);
    va_end(args);
}

static const char* get_windows_network_error(int last_error) {
    FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL,
                  last_error,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  windows_error_str,
                  NET_ERROR_MESSAGE_LEN,
                  NULL);
    return windows_error_str;
}

const char * GetNetError(void)
{
    return err_str;
}

bool InitNetwork(const char * log_name)
{
    WORD wsa_version_requested = MAKEWORD(2, 2);
    WSADATA wsa_data = {0};
    int rc = WSAStartup(wsa_version_requested, &wsa_data);
    if (rc != 0) {
        int last_error = WSAGetLastError();
        set_err("WSAStartup() failed with %s\n", get_windows_network_error(last_error));
        return false;
    }

    log_file = fopen(log_name, "w");
    if (log_file == NULL) {
        set_err("Failed to open net.log\n");
        return false;
    }

    return true;
}

Socket CreateClient(const char * ip, const char * port)
{
    // TODO: Consider consolidating with mac version of func.
    assert(port != NULL);
    assert(ip != NULL);

    Socket result = { 0 };
    BufferInit(&result.write_buf, 1024); // TODO: extract init code commont to both client and server

    struct addrinfo hints = {
        .ai_family = AF_UNSPEC, // don't care IPv4 or IPv6
        .ai_socktype = SOCK_STREAM // TCP stream sockets
    };

    struct addrinfo *server_info;  // will point to the results

    // get ready to connect
    int rc = getaddrinfo(ip, port, &hints, &server_info);
    if (rc != 0) {
        set_err("getaddrinfo error: %s\n", gai_strerror(rc));
        goto done;
    }

    // Create client socket file descriptor.
    result.fd = socket(server_info->ai_family,
                       server_info->ai_socktype,
                       (int)server_info->ai_protocol);

    if (result.fd < 0) {
        int last_error = WSAGetLastError();
        set_err("socket() failed: %s\n", get_windows_network_error(last_error));
        goto done;
    }

    u_long socket_flags = 1;
    rc = ioctlsocket(result.fd, FIONBIO, &socket_flags);
    if (rc != NO_ERROR) {
        int last_error = WSAGetLastError();
        set_err("ioctlsocket() failed with error: %s\n", get_windows_network_error(last_error));
        goto done;
    }

    rc = connect(result.fd, server_info->ai_addr, (int)server_info->ai_addrlen);
    if (rc == SOCKET_ERROR) {
        // Since the socket is non-blocking, the connect() operation is also non-blocking. If the
        // error that we receive is that it would block, then we just need to wait for the
        // connection to finish establishing through connect. On any other error just return that
        // error.
        int last_error = WSAGetLastError();
        if (last_error != WSAEWOULDBLOCK) {
            set_err("connect error: %s\n", get_windows_network_error(last_error));
            return NULL;
        }

        fd_set writeable_sockets;
        struct timeval timeout = {5, 0};

        FD_ZERO(&writeable_sockets);
        FD_SET(sock->socket, &writeable_sockets);

        rc = select(0, NULL, &writeable_sockets, NULL, &timeout);
        if (rc == 0) {
            set_err("timed out connecting to server");
            return NULL;
        } else if (rc == SOCKET_ERROR) {
            last_error = WSAGetLastError();
            set_err("connect error: %s\n", get_windows_network_error(last_error));
            return NULL;
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
    assert(port != NULL);

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
        set_err("getaddrinfo error: %s\n", gai_strerror(rc));
        goto done;
    }

    result.fd = socket(server_info->ai_family,
                       server_info->ai_socktype,
                       server_info->ai_protocol);
    if (result.fd < 0) {
        int last_error = WSAGetLastError();
        set_err("socket() failed: %s\n", get_windows_network_error(last_error));
        goto error;
    }

    // Set socket file descriptor to non blocking, so we can call accept() without blocking.
    u_long server_socket_flags = 1;
    rc = ioctlsocket(sock->socket, FIONBIO, &server_socket_flags);
    if (rc != NO_ERROR) {
        int last_error = WSAGetLastError();
        set_err("ioctlsocket() failed with error: %s\n", get_windows_network_error(last_error));
        goto error;
    }

    // Bind to a specific port.
    rc = bind(result.fd,
              server_info->ai_addr,
              (int)server_info->ai_addrlen);
    if (rc != 0) {
        int last_error = WSAGetLastError();
        set_err("bind() failed: %s\n", get_windows_network_error(last_error));
        goto error;
    }

    // Listen on the socket for incoming connections.
    rc = listen(result.fd, SERVER_ACCEPT_QUEUE_LIMIT);
    if (rc != 0) {
        int last_error = WSAGetLastError();
        set_err("listen() failed: %s\n", get_windows_network_error(last_error));
        goto error;
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
    assert(server != NULL);
    assert(out != NULL);

    out->fd = accept(server->socket, NULL, NULL);
    out->is_init = false;

    // socket is still -1 on error:
    if (sock == -1) {
        int last_error = WSAGetLastError();
        if (last_error == WSAEWOULDBLOCK) {
            // accept didn't fail, but there was no connection:
            *out = NULL;
            return true;
        }

        // accept failed:
        set_err("accept() failed: %s\n", get_windows_network_error(last_error));
        return false;
    }

    // there was a connection.
    BufferInit(&out->write_buf, 1024);
    out->is_init = true;
    return true;
}

int NetWrite(const Socket * socket, void * data, int size)
{
    assert(socket != NULL);
    assert(buf != NULL);
    assert(size > 0);

    int size_sent = send(socket->fd, (char*)buf, size, 0);
    if (size_sent == -1) {
        int last_error = WSAGetLastError();
        if (last_error == WSAEWOULDBLOCK) {
            return 0;
        }

        set_err("Failed to send data: %s\n", get_windows_network_error(last_error));
        return -1;
    }

    return size_sent;
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
    assert(socket != NULL);
    assert(buf != NULL);
    assert(size > 0);

    int received = recv(socket->fd, (char*)buf, size, 0);
    if ( received < 0 ) {
        int last_error = WSAGetLastError();
        if (last_error == WSAEWOULDBLOCK) {
            // Received nothing, but socket was non-blocking so it's okay.
            return 0;
        }

        set_err("Error receiving data: %s\n", get_windows_network_error(last_error));
        return -1;
    }

    return received;
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

void ShutdownNet(void)
{
    fclose(log_file);
}

void NetLog(const char * format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(log_file, format, args);
    va_end(args);
    // TODO: Remote this, just added for testing.
    fflush(log_file);
}
