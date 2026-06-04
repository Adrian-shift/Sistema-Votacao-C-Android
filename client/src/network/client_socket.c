#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#include "client_socket.h"
#include "ssl_wrapper.h"

#define CONNECT_TIMEOUT_SECONDS 5

static int is_ip_address(const char *value)
{
    struct in_addr addr4;
    struct in6_addr addr6;

    if(!value || value[0] == '\0')
    {
        return 0;
    }

    return inet_pton(AF_INET, value, &addr4) == 1 ||
           inet_pton(AF_INET6, value, &addr6) == 1;
}

static void set_error_message(
    char *error_buffer,
    size_t error_buffer_size,
    const char *message
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

ssl_connection_t* connect_server_ssl(
    const char *ip,
    int port,
    char *error_buffer,
    size_t error_buffer_size
)
{
    int sock;

    struct sockaddr_in server_addr;

    if(error_buffer && error_buffer_size > 0)
    {
        error_buffer[0] = '\0';
    }

    sock = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    if(sock < 0)
    {
        set_error_message(
            error_buffer,
            error_buffer_size,
            "Falha ao criar socket"
        );
        return NULL;
    }

    server_addr.sin_family = AF_INET;

    server_addr.sin_port = htons(port);

    if(inet_pton(
        AF_INET,
        ip,
        &server_addr.sin_addr
    ) != 1)
    {
        set_error_message(
            error_buffer,
            error_buffer_size,
            "IP invalido"
        );
        close(sock);
        return NULL;
    }

    int flags = fcntl(sock, F_GETFL, 0);

    if(flags < 0)
    {
        set_error_message(
            error_buffer,
            error_buffer_size,
            "Falha ao consultar estado do socket"
        );
        close(sock);
        return NULL;
    }

    if(fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0)
    {
        set_error_message(
            error_buffer,
            error_buffer_size,
            "Falha ao configurar socket"
        );
        close(sock);
        return NULL;
    }

    int result = connect(
        sock,
        (struct sockaddr*)&server_addr,
        sizeof(server_addr)
    );

    if(result < 0 && errno != EINPROGRESS)
    {
        set_error_message(
            error_buffer,
            error_buffer_size,
            "Falha ao conectar ao servidor"
        );
        close(sock);
        return NULL;
    }

    if(result < 0)
    {
        fd_set write_fds;

        FD_ZERO(&write_fds);
        FD_SET(sock, &write_fds);

        struct timeval timeout;

        timeout.tv_sec = CONNECT_TIMEOUT_SECONDS;
        timeout.tv_usec = 0;

        result = select(
            sock + 1,
            NULL,
            &write_fds,
            NULL,
            &timeout
        );

        if(result <= 0)
        {
            set_error_message(
                error_buffer,
                error_buffer_size,
                "Timeout na conexao"
            );
            close(sock);
            return NULL;
        }

        int socket_error = 0;
        socklen_t socket_error_len = sizeof(socket_error);

        if(getsockopt(
            sock,
            SOL_SOCKET,
            SO_ERROR,
            &socket_error,
            &socket_error_len
        ) < 0 || socket_error != 0)
        {
            set_error_message(
                error_buffer,
                error_buffer_size,
                "Falha ao verificar estado da conexao"
            );
            close(sock);
            return NULL;
        }
    }

    if(fcntl(sock, F_SETFL, flags) < 0)
    {
        set_error_message(
            error_buffer,
            error_buffer_size,
            "Falha ao restaurar socket"
        );
        close(sock);
        return NULL;
    }

    SSL_CTX* ssl_ctx = init_client_ssl(
        error_buffer,
        error_buffer_size
    );
    if(!ssl_ctx)
    {
        close(sock);
        return NULL;
    }

    SSL* ssl = ssl_connect(
        ssl_ctx,
        sock,
        ip,
        is_ip_address(ip),
        error_buffer,
        error_buffer_size
    );
    if(!ssl)
    {
        close(sock);
        ssl_cleanup(NULL, ssl_ctx);
        return NULL;
    }

    ssl_connection_t* conn = malloc(sizeof(ssl_connection_t));
    if(!conn)
    {
        close(sock);
        ssl_cleanup(ssl, ssl_ctx);
        return NULL;
    }

    conn->socket_fd = sock;
    conn->ssl = ssl;
    conn->ssl_ctx = ssl_ctx;

    return conn;
}

void close_ssl_connection(ssl_connection_t* conn)
{
    if(conn)
    {
        if(conn->ssl)
        {
            SSL_shutdown(conn->ssl);
            SSL_free(conn->ssl);
        }
        if(conn->ssl_ctx)
        {
            SSL_CTX_free(conn->ssl_ctx);
        }
        if(conn->socket_fd >= 0)
        {
            close(conn->socket_fd);
        }
        free(conn);
    }
}

int send_vote(
    ssl_connection_t* conn,
    const char *voter_id,
    const char *candidate,
    char *response
)
{
    char message[256];

    snprintf(
        message,
        sizeof(message),
        "%s %s",
        voter_id,
        candidate
    );

    ssl_send(
        conn->ssl,
        message,
        strlen(message)
    );

    int bytes = ssl_recv(
        conn->ssl,
        response,
        255
    );

    if(bytes <= 0)
    {
        return 0;
    }

    response[bytes] = '\0';

    return 1;
}
