#include <arpa/inet.h>

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../database/database.h"
#include "../models/server_state.h"
#include "../protocol/protocol.h"
#include "../security/rate_limit.h"
#include "../security/secure_log.h"
#include "../security/validation.h"

#include "ssl_wrapper.h"

#define PORT 8080
#define DEFAULT_CERT_FILE "../certs/server.crt"
#define DEFAULT_KEY_FILE "../certs/server.key"

typedef struct {
    int client_socket;
    SSL_CTX* ssl_ctx;
    char client_ip[INET_ADDRSTRLEN];
} client_args_t;

void* handle_client(void* arg)
{
    client_args_t* args = (client_args_t*)arg;
    int client_socket = args->client_socket;
    SSL_CTX* ssl_ctx = args->ssl_ctx;
    char client_ip[INET_ADDRSTRLEN];

    snprintf(
        client_ip,
        sizeof(client_ip),
        "%s",
        args->client_ip
    );

    free(args);

    char ssl_error[256];
    SSL* ssl = ssl_accept(
        ssl_ctx,
        client_socket,
        ssl_error,
        sizeof(ssl_error)
    );
    if(!ssl)
    {
        close(client_socket);

        {
            char log[512];

            snprintf(
                log,
                sizeof(log),
                "Falha no handshake SSL [%s]%s%s",
                client_ip,
                ssl_error[0] ? ": " : "",
                ssl_error[0] ? ssl_error : ""
            );

            security_log_event("SSL", log);
        }

        return NULL;
    }

    char buffer[1024];
    int bytes;

    server_state_lock();
    server_state.connected_clients++;
    server_state_unlock();

    {
        char log[512];

        snprintf(
            log,
            sizeof(log),
            "\n[INFO] Novo cliente conectado (SSL) [%s]",
            client_ip
        );

        add_log(log);
    }

    while((bytes = ssl_recv(ssl, buffer, sizeof(buffer) - 1)) > 0)
    {
        buffer[bytes] = '\0';

        char voter_id[64];
        char candidate[64];

        if(!parse_vote_message(
            buffer,
            voter_id,
            sizeof(voter_id),
            candidate,
            sizeof(candidate)
        ))
        {
            ssl_send(
                ssl,
                "NACK FORMATO_INVALIDO\n",
                22
            );

            security_log_event(
                "VALIDACAO",
                "Mensagem de voto invalida"
            );

            continue;
        }

        char log[512];

        snprintf(
            log,
            sizeof(log),
            "[VOTO] Eleitor %s -> %s",
            voter_id,
            candidate
        );

        add_log(log);

        if(!voter_exists(voter_id))
        {
            ssl_send(
                ssl,
                "NACK ELEITOR_INVALIDO\n",
                23
            );

            security_log_event(
                "VALIDACAO",
                "Eleitor invalido"
            );

            continue;
        }

        if(has_voted(voter_id))
        {
            ssl_send(
                ssl,
                "NACK JA_VOTOU\n",
                16
            );

            security_log_event(
                "REGRA",
                "Eleitor ja votou"
            );

            continue;
        }

        char receipt[128];

        generate_receipt(receipt);

        if(save_vote(
            voter_id,
            candidate,
            receipt
        ))
        {
            server_state_lock();
            server_state.total_votes++;
            server_state_unlock();

            char response[256];

            snprintf(
                response,
                sizeof(response),
                "ACK %s\n",
                receipt
            );

            ssl_send(
                ssl,
                response,
                strlen(response)
            );

            add_log("[INFO] Voto registrado");
        }
        else
        {
            ssl_send(
                ssl,
                "NACK ERRO_DB\n",
                14
            );

            security_log_event(
                "DB",
                "Falha ao salvar voto"
            );
        }
    }

    ssl_cleanup(ssl, NULL);
    close(client_socket);

    server_state_lock();
    server_state.connected_clients--;
    server_state.active_threads--;
    server_state_unlock();

    {
        char log[512];

        snprintf(
            log,
            sizeof(log),
            "[INFO] Cliente desconectado [%s]",
            client_ip
        );

        add_log(log);
    }

    return NULL;
}

void* start_server(void* arg)
{
    char ssl_error[256];

    SSL_CTX* ssl_ctx = init_server_ssl(
        DEFAULT_CERT_FILE,
        DEFAULT_KEY_FILE,
        ssl_error,
        sizeof(ssl_error)
    );
    if(!ssl_ctx)
    {
        {
            char log[512];

            snprintf(
                log,
                sizeof(log),
                "Falha ao inicializar SSL/TLS%s%s",
                ssl_error[0] ? ": " : "",
                ssl_error[0] ? ssl_error : ""
            );

            security_log_event("SSL", log);
        }

        return NULL;
    }

    add_log("[INFO] SSL/TLS inicializado");

    int server_socket;

    struct sockaddr_in server_addr;

    server_socket = socket(
        AF_INET,
        SOCK_STREAM,
        0
    );

    if(server_socket < 0)
    {
        add_log("[ERRO] Falha ao criar socket");
        ssl_cleanup(NULL, ssl_ctx);
        return NULL;
    }

    server_addr.sin_family = AF_INET;

    server_addr.sin_port = htons(PORT);

    server_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(
        server_socket,
        (struct sockaddr*)&server_addr,
        sizeof(server_addr)
    ) < 0)
    {
        add_log("[ERRO] Bind falhou");
        close(server_socket);
        ssl_cleanup(NULL, ssl_ctx);
        return NULL;
    }

    if(listen(server_socket, 10) < 0)
    {
        add_log("[ERRO] Listen falhou");
        close(server_socket);
        ssl_cleanup(NULL, ssl_ctx);
        return NULL;
    }

    add_log("[INFO] Servidor TCP iniciado na porta 8080 (SSL/TLS)");

    while(1)
    {
        int client_socket;

        struct sockaddr_in client_addr;

        socklen_t client_len = sizeof(client_addr);

        client_socket = accept(
            server_socket,
            (struct sockaddr*)&client_addr,
            &client_len
        );

        if(client_socket < 0)
        {
            add_log("[ERRO] Accept falhou");
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        if(!inet_ntop(
            AF_INET,
            &client_addr.sin_addr,
            client_ip,
            sizeof(client_ip)
        ))
        {
            snprintf(
                client_ip,
                sizeof(client_ip),
                "desconhecido"
            );
        }

        if(!rate_limit_allow(client_ip))
        {
            security_log_event("RATE_LIMIT", client_ip);
            close(client_socket);
            continue;
        }

        pthread_t client_thread;

        client_args_t* args = malloc(sizeof(client_args_t));
        if(!args)
        {
            close(client_socket);
            security_log_event("MEMORIA", "Falha ao alocar argumentos do cliente");
            continue;
        }

        args->client_socket = client_socket;
        args->ssl_ctx = ssl_ctx;
        snprintf(
            args->client_ip,
            sizeof(args->client_ip),
            "%s",
            client_ip
        );

        server_state_lock();
        server_state.active_threads++;
        server_state_unlock();

        if(pthread_create(
            &client_thread,
            NULL,
            handle_client,
            args
        ) != 0)
        {
            server_state_lock();
            server_state.active_threads--;
            server_state_unlock();
            close(client_socket);
            security_log_event("THREAD", "Falha ao criar thread do cliente");
            free(args);
            continue;
        }

        pthread_detach(client_thread);
    }

    close(server_socket);
    ssl_cleanup(NULL, ssl_ctx);
    return NULL;
}
