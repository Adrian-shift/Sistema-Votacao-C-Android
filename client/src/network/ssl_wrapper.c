#include "ssl_wrapper.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/pem.h>
#include <openssl/x509v3.h>

#include "embedded_ca.h"

static void set_error_message(
    char* error_buffer,
    size_t error_buffer_size,
    const char* message
)
{
    if(!error_buffer || error_buffer_size == 0)
    {
        return;
    }

    snprintf(
        error_buffer,
        error_buffer_size,
        "%s",
        message ? message : "erro desconhecido"
    );
}

static void capture_openssl_error(
    char* error_buffer,
    size_t error_buffer_size,
    const char* fallback_message
)
{
    unsigned long error_code;

    if(!error_buffer || error_buffer_size == 0)
    {
        return;
    }

    error_code = ERR_get_error();
    if(error_code == 0)
    {
        set_error_message(
            error_buffer,
            error_buffer_size,
            fallback_message
        );
        return;
    }

    ERR_error_string_n(
        error_code,
        error_buffer,
        error_buffer_size
    );
}

static void capture_ssl_handshake_error(
    SSL* ssl,
    int result,
    char* error_buffer,
    size_t error_buffer_size,
    const char* fallback_message
)
{
    unsigned long error_code;
    int ssl_error;

    if(!error_buffer || error_buffer_size == 0)
    {
        return;
    }

    ssl_error = SSL_get_error(ssl, result);
    {
        long verify_result = SSL_get_verify_result(ssl);

        if(verify_result != X509_V_OK)
        {
            snprintf(
                error_buffer,
                error_buffer_size,
                "%s: %s",
                fallback_message,
                X509_verify_cert_error_string(verify_result)
            );
            return;
        }
    }

    error_code = ERR_get_error();

    if(error_code != 0)
    {
        ERR_error_string_n(
            error_code,
            error_buffer,
            error_buffer_size
        );
        return;
    }

    if(ssl_error == SSL_ERROR_SYSCALL && errno != 0)
    {
        snprintf(
            error_buffer,
            error_buffer_size,
            "%s: %s",
            fallback_message,
            strerror(errno)
        );
        return;
    }

    snprintf(
        error_buffer,
        error_buffer_size,
        "%s (SSL_get_error=%d)",
        fallback_message,
        ssl_error
    );
}

static int configure_peer_verification(
    SSL* ssl,
    const char* peer_name,
    int peer_is_ip,
    char* error_buffer,
    size_t error_buffer_size
)
{
    X509_VERIFY_PARAM* param;

    if(!ssl || !peer_name || peer_name[0] == '\0')
    {
        return 1;
    }

    param = SSL_get0_param(ssl);
    if(!param)
    {
        set_error_message(
            error_buffer,
            error_buffer_size,
            "Falha ao configurar verificacao do servidor"
        );
        return 0;
    }

    if(peer_is_ip)
    {
        if(X509_VERIFY_PARAM_set1_ip_asc(param, peer_name) != 1)
        {
            set_error_message(
                error_buffer,
                error_buffer_size,
                "Falha ao validar o endereco IP do servidor"
            );
            return 0;
        }

        return 1;
    }

    if(X509_VERIFY_PARAM_set1_host(param, peer_name, 0) != 1)
    {
        set_error_message(
            error_buffer,
            error_buffer_size,
            "Falha ao validar o nome do servidor"
        );
        return 0;
    }

    if(SSL_set_tlsext_host_name(ssl, peer_name) != 1)
    {
        set_error_message(
            error_buffer,
            error_buffer_size,
            "Falha ao configurar SNI"
        );
        return 0;
    }

    return 1;
}

static int load_embedded_ca(
    SSL_CTX* ctx,
    char* error_buffer,
    size_t error_buffer_size
)
{
    BIO* bio;
    X509* ca_certificate;
    X509_STORE* store;

    if(!ctx)
    {
        return 0;
    }

    if(EMBEDDED_CA_PEM[0] == '\0')
    {
        set_error_message(
            error_buffer,
            error_buffer_size,
            "CA embutida nao encontrada. Rode scripts/generate_certs.sh."
        );
        return 0;
    }

    bio = BIO_new_mem_buf(EMBEDDED_CA_PEM, -1);
    if(!bio)
    {
        capture_openssl_error(
            error_buffer,
            error_buffer_size,
            "Falha ao criar BIO da CA embutida"
        );
        return 0;
    }

    ca_certificate = PEM_read_bio_X509(bio, NULL, 0, NULL);
    if(!ca_certificate)
    {
        capture_openssl_error(
            error_buffer,
            error_buffer_size,
            "Falha ao ler a CA embutida"
        );
        BIO_free(bio);
        return 0;
    }

    store = SSL_CTX_get_cert_store(ctx);
    if(!store)
    {
        set_error_message(
            error_buffer,
            error_buffer_size,
            "Falha ao acessar armazenamento de certificados"
        );
        X509_free(ca_certificate);
        BIO_free(bio);
        return 0;
    }

    if(X509_STORE_add_cert(store, ca_certificate) != 1)
    {
        capture_openssl_error(
            error_buffer,
            error_buffer_size,
            "Falha ao adicionar CA embutida"
        );
        X509_free(ca_certificate);
        BIO_free(bio);
        return 0;
    }

    X509_free(ca_certificate);
    BIO_free(bio);
    return 1;
}

SSL_CTX* init_client_ssl(
    char* error_buffer,
    size_t error_buffer_size
)
{
    SSL_CTX* ctx;

    if(error_buffer && error_buffer_size > 0)
    {
        error_buffer[0] = '\0';
    }

    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    const SSL_METHOD* method = TLS_client_method();

    ctx = SSL_CTX_new(method);
    if(!ctx)
    {
        capture_openssl_error(
            error_buffer,
            error_buffer_size,
            "Falha ao criar SSL_CTX"
        );
        return NULL;
    }

    SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
    SSL_CTX_set_max_proto_version(ctx, TLS1_3_VERSION);

    if(!load_embedded_ca(
        ctx,
        error_buffer,
        error_buffer_size
    ))
    {
        SSL_CTX_free(ctx);
        return NULL;
    }

    SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, NULL);

    return ctx;
}

SSL* ssl_connect(
    SSL_CTX* ctx,
    int socket_fd,
    const char* peer_name,
    int peer_is_ip,
    char* error_buffer,
    size_t error_buffer_size
)
{
    if(error_buffer && error_buffer_size > 0)
    {
        error_buffer[0] = '\0';
    }

    ERR_clear_error();

    SSL* ssl = SSL_new(ctx);
    int result;

    if(!ssl)
    {
        capture_openssl_error(
            error_buffer,
            error_buffer_size,
            "Falha ao criar SSL"
        );
        return NULL;
    }

    SSL_set_fd(ssl, socket_fd);

    if(!configure_peer_verification(
        ssl,
        peer_name,
        peer_is_ip,
        error_buffer,
        error_buffer_size
    ))
    {
        SSL_free(ssl);
        return NULL;
    }

    result = SSL_connect(ssl);

    if(result <= 0)
    {
        capture_ssl_handshake_error(
            ssl,
            result,
            error_buffer,
            error_buffer_size,
            "Falha no handshake SSL"
        );
        SSL_free(ssl);
        return NULL;
    }

    return ssl;
}

void ssl_cleanup(SSL* ssl, SSL_CTX* ctx)
{
    if(ssl)
    {
        SSL_shutdown(ssl);
        SSL_free(ssl);
    }

    if(ctx)
    {
        SSL_CTX_free(ctx);
    }
}

int ssl_send(SSL* ssl, const char* data, int len)
{
    return SSL_write(ssl, data, len);
}

int ssl_recv(SSL* ssl, char* buffer, int len)
{
    return SSL_read(ssl, buffer, len);
}
