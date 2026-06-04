#include <stdio.h>

#include <unistd.h>

#include <newt.h>

#include "ui/login.h"

#include "ui/voting.h"

#include "ui/receipt.h"

#include "network/client_socket.h"

int main()
{
    char ip[64];

    int port;

    char voter_id[64];

    char candidate[64];

    char response[256];
    char connect_error[256];

    newtInit();

    newtCls();

    show_login(
        ip,
        &port,
        voter_id
    );

        ssl_connection_t* conn =
        connect_server_ssl(
            ip,
            port,
            connect_error,
            sizeof(connect_error)
        );

    if(!conn)
    {
        newtWinMessage(
            "Erro",
            "OK",
            connect_error[0] ? connect_error : "Falha ao conectar (SSL)"
        );

        newtFinished();

        return 1;
    }

    show_voting_menu(candidate);

    if(send_vote(
        conn,
        voter_id,
        candidate,
        response
    ))
    {
        show_receipt(response);
    }
    else
    {
        newtWinMessage(
            "Erro",
            "OK",
            "Falha ao enviar voto"
        );
    }

    close_ssl_connection(conn);

    newtFinished();

    return 0;
}
