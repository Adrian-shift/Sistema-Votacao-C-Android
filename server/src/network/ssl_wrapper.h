#ifndef SSL_WRAPPER_H
#define SSL_WRAPPER_H

#include <stddef.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

SSL_CTX* init_server_ssl(
    const char* cert_file,
    const char* key_file,
    char* error_buffer,
    size_t error_buffer_size
);
SSL* ssl_accept(
    SSL_CTX* ctx,
    int client_socket,
    char* error_buffer,
    size_t error_buffer_size
);
void ssl_cleanup(SSL* ssl, SSL_CTX* ctx);
int ssl_send(SSL* ssl, const char* data, int len);
int ssl_recv(SSL* ssl, char* buffer, int len);

#endif
