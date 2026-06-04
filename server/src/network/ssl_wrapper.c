#include "ssl_wrapper.h"

#include <limits.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

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

static int resolve_runtime_path(
    const char* relative_path,
    char* resolved_path,
    size_t resolved_path_size
)
{
    char exe_path[PATH_MAX];
    char candidate[PATH_MAX];
    ssize_t len;
    char* slash;

    if(!relative_path || !resolved_path || resolved_path_size == 0)
    {
        return 0;
    }

    if(relative_path[0] == '/')
    {
        if(access(relative_path, R_OK) == 0)
        {
            snprintf(resolved_path, resolved_path_size, "%s", relative_path);
            return 1;
        }

        return 0;
    }

    len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if(len < 0)
    {
        return 0;
    }

    exe_path[len] = '\0';

    slash = strrchr(exe_path, '/');
    if(!slash)
    {
        return 0;
    }

    *slash = '\0';

    if(snprintf(
        candidate,
        sizeof(candidate),
        "%s/../%s",
        exe_path,
        relative_path
    ) >= (int)sizeof(candidate))
    {
        return 0;
    }

    if(access(candidate, R_OK) != 0)
    {
        return 0;
    }

    snprintf(resolved_path, resolved_path_size, "%s", candidate);
    return 1;
}

SSL_CTX* init_server_ssl(
    const char* cert_file,
    const char* key_file,
    char* error_buffer,
    size_t error_buffer_size
)
{
    SSL_CTX* ctx;
    char resolved_cert_file[PATH_MAX];
    char resolved_key_file[PATH_MAX];

    if(error_buffer && error_buffer_size > 0)
    {
        error_buffer[0] = '\0';
    }

    SSL_library_init();
    SSL_load_error_strings();
    OpenSSL_add_all_algorithms();

    const SSL_METHOD* method = TLS_server_method();

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

    if(!resolve_runtime_path(
        cert_file,
        resolved_cert_file,
        sizeof(resolved_cert_file)
    ))
    {
        set_error_message(
            error_buffer,
            error_buffer_size,
            "Certificado do servidor nao encontrado"
        );
        SSL_CTX_free(ctx);
        return NULL;
    }

    if(SSL_CTX_use_certificate_file(
        ctx,
        resolved_cert_file,
        SSL_FILETYPE_PEM
    ) <= 0)
    {
        capture_openssl_error(
            error_buffer,
            error_buffer_size,
            "Falha ao carregar certificado do servidor"
        );
        SSL_CTX_free(ctx);
        return NULL;
    }

    if(!resolve_runtime_path(
        key_file,
        resolved_key_file,
        sizeof(resolved_key_file)
    ))
    {
        set_error_message(
            error_buffer,
            error_buffer_size,
            "Chave privada do servidor nao encontrada"
        );
        SSL_CTX_free(ctx);
        return NULL;
    }

    if(SSL_CTX_use_PrivateKey_file(
        ctx,
        resolved_key_file,
        SSL_FILETYPE_PEM
    ) <= 0)
    {
        capture_openssl_error(
            error_buffer,
            error_buffer_size,
            "Falha ao carregar chave privada do servidor"
        );
        SSL_CTX_free(ctx);
        return NULL;
    }

    if(!SSL_CTX_check_private_key(ctx))
    {
        set_error_message(
            error_buffer,
            error_buffer_size,
            "Chave privada nao corresponde ao certificado"
        );
        SSL_CTX_free(ctx);
        return NULL;
    }

    return ctx;
}

SSL* ssl_accept(
    SSL_CTX* ctx,
    int client_socket,
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

    SSL_set_fd(ssl, client_socket);

    result = SSL_accept(ssl);

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
