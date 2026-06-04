#ifndef CLIENT_SOCKET_H
#define CLIENT_SOCKET_H

#include <stddef.h>
#include <openssl/ssl.h>

typedef struct {
    int socket_fd;
    SSL* ssl;
    SSL_CTX* ssl_ctx;
} ssl_connection_t;

ssl_connection_t* connect_server_ssl(
    const char *ip,
    int port,
    char *error_buffer,
    size_t error_buffer_size
);

void close_ssl_connection(ssl_connection_t* conn);

int send_vote(
    ssl_connection_t* conn,
    const char *voter_id,
    const char *candidate,
    char *response
);

#endif
