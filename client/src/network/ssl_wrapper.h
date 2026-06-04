#ifndef SSL_WRAPPER_H
#define SSL_WRAPPER_H

#include <stddef.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

SSL_CTX* init_client_ssl(
    char* error_buffer,
    size_t error_buffer_size
);
SSL* ssl_connect(
    SSL_CTX* ctx,
    int socket_fd,
    const char* peer_name,
    int peer_is_ip,
    char* error_buffer,
    size_t error_buffer_size
);
void ssl_cleanup(SSL* ssl, SSL_CTX* ctx);
int ssl_send(SSL* ssl, const char* data, int len);
int ssl_recv(SSL* ssl, char* buffer, int len);

#endif
