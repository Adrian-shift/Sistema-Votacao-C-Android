#include <newt.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "../database/database.h"
#include "../models/server_state.h"
#include "../network/server_socket.h"
#include "../security/auth.h"
#include "dashboard.h"
#include "results.h"

static int running = 1;

void show_popup(char* title, char* message)
{
    newtWinMessage(
        title,
        "OK",
        message
    );
}

static void show_access_denied()
{
    show_popup(
        "Acesso negado",
        "Senha administrativa incorreta"
    );
}

static void show_text_window(
    const char *title,
    const char *text,
    int width,
    int height
)
{
    newtComponent form;
    newtComponent box;
    newtComponent btn_close;

    newtCenteredWindow(
        width,
        height,
        title
    );

    box = newtTextbox(
        2,
        2,
        width - 4,
        height - 6,
        NEWT_FLAG_BORDER | NEWT_FLAG_SCROLL
    );

    btn_close = newtButton(
        (width - 8) / 2,
        height - 3,
        "Voltar"
    );

    newtTextboxSetText(
        box,
        text
    );

    form = newtForm(NULL, NULL, 0);

    newtFormAddComponents(
        form,
        box,
        btn_close,
        NULL
    );

    newtRunForm(form);

    newtFormDestroy(form);
    newtPopWindow();
}

static void collect_logs_text(char *buffer, size_t size)
{
    size_t used = 0;

    if(size == 0)
    {
        return;
    }

    buffer[0] = '\0';

    server_state_lock();

    for(int i = 0; i < server_state.log_count; i++)
    {
        size_t remaining = size - used;

        if(remaining <= 1)
        {
            break;
        }

        int written = snprintf(
            buffer + used,
            remaining,
            "%s\n",
            server_state.logs[i]
        );

        if(written < 0)
        {
            break;
        }

        if((size_t)written >= remaining)
        {
            used = size - 1;
            break;
        }

        used += (size_t)written;
    }

    server_state_unlock();

    if(used == 0)
    {
        snprintf(
            buffer,
            size,
            "Nenhum log registrado ainda.\n"
        );
    }
}

static void show_logs_popup(void)
{
    char logs[8192];
    char text[8192];
    int connected_clients;
    int active_threads;
    int total_votes;

    server_state_lock();
    connected_clients = server_state.connected_clients;
    active_threads = server_state.active_threads;
    server_state_unlock();

    total_votes = count_votes();

    collect_logs_text(
        logs,
        sizeof(logs)
    );

    snprintf(
        text,
        sizeof(text),
        "Logs recentes do servidor\n\n"
        "Clientes conectados: %d\n"
        "Threads ativas: %d\n"
        "Votos registrados: %d\n\n"
        "%s",
        connected_clients,
        active_threads,
        total_votes,
        logs
    );

    show_text_window(
        "Logs",
        text,
        66,
        20
    );
}

static void show_clients_popup(void)
{
    char text[512];
    int connected_clients;
    int active_threads;
    int total_votes;

    server_state_lock();
    connected_clients = server_state.connected_clients;
    active_threads = server_state.active_threads;
    server_state_unlock();

    total_votes = count_votes();

    snprintf(
        text,
        sizeof(text),
        "Resumo de clientes\n\n"
        "Clientes conectados atualmente: %d\n"
        "Threads ativas: %d\n"
        "Votos registrados: %d\n\n"
        "Use F2 para ver os logs detalhados.",
        connected_clients,
        active_threads,
        total_votes
    );

    show_text_window(
        "Clientes",
        text,
        52,
        14
    );
}

void update_dashboard_text(
    newtComponent status_box,
    newtComponent logs_box
)
{
    char status[512];
    char logs[8192] = "";
    int total_votes = count_votes();
    int connected_clients;
    int active_threads;
    int log_count;
    size_t used = 0;

    server_state_lock();

    connected_clients = server_state.connected_clients;
    active_threads = server_state.active_threads;
    log_count = server_state.log_count;

    for(int i = 0; i < log_count; i++)
    {
        int written = snprintf(
            logs + used,
            sizeof(logs) - used,
            "%s\n",
            server_state.logs[i]
        );

        if(written < 0 || (size_t)written >= sizeof(logs) - used)
        {
            break;
        }

        used += (size_t)written;
    }

    server_state_unlock();

    snprintf(
        status,
        sizeof(status),
        "Clientes: %d\n\n"
        "Votos: %d\n\n"
        "Threads: %d\n",
        connected_clients,
        total_votes,
        active_threads
    );

    newtTextboxSetText(
        status_box,
        status
    );

    newtTextboxSetText(
        logs_box,
        logs
    );

    newtRefresh();
}

void start_dashboard()
{
    add_log("[INFO] Servidor iniciado");
    add_log("[INFO] Interface carregada");
    add_log("[INFO] Sistema pronto");

    pthread_t network_thread;

    pthread_create(
        &network_thread,
        NULL,
        start_server,
        NULL
    );

    newtComponent form;
    newtComponent lbl_title;
    newtComponent status_box;
    newtComponent logs_box;
    newtComponent btn_reports;
    newtComponent btn_logs;
    newtComponent btn_clients;
    newtComponent btn_exit;

    newtCenteredWindow(
        68,
        22,
        "Servidor de Votacao"
    );

    lbl_title = newtLabel(
        2,
        1,
        "SERVIDOR ONLINE"
    );

    status_box = newtTextbox(
        2,
        3,
        18,
        12,
        NEWT_FLAG_BORDER
    );

    logs_box = newtTextbox(
        22,
        3,
        42,
        14,
        NEWT_FLAG_SCROLL | NEWT_FLAG_BORDER
    );

    btn_reports = newtButton(
        2,
        19,
        "F1"
    );

    btn_logs = newtButton(
        14,
        19,
        "F2"
    );

    btn_clients = newtButton(
        26,
        19,
        "F3"
    );

    btn_exit = newtButton(
        50,
        19,
        "Q Sair"
    );

    form = newtForm(NULL, NULL, 0);

    newtFormAddComponents(
        form,
        lbl_title,
        status_box,
        logs_box,
        btn_reports,
        btn_logs,
        btn_clients,
        btn_exit,
        NULL
    );

    newtFormAddHotKey(
        form,
        NEWT_KEY_F1
    );

    newtFormAddHotKey(
        form,
        NEWT_KEY_F2
    );

    newtFormAddHotKey(
        form,
        NEWT_KEY_F3
    );

    while(running)
    {
        struct newtExitStruct exit_status;

        update_dashboard_text(
            status_box,
            logs_box
        );

        newtFormSetTimer(
            form,
            1000
        );

        newtFormRun(
            form,
            &exit_status
        );

        if(exit_status.reason == NEWT_EXIT_HOTKEY)
        {
            if(exit_status.u.key == NEWT_KEY_F1)
            {
                if(admin_action_allowed("Resultados"))
                {
                    show_results();
                }
                else
                {
                    show_access_denied();
                }
            }
            else if(exit_status.u.key == NEWT_KEY_F2)
            {
                if(admin_action_allowed("Logs"))
                {
                    show_logs_popup();
                }
                else
                {
                    show_access_denied();
                }
            }
            else if(exit_status.u.key == NEWT_KEY_F3)
            {
                if(admin_action_allowed("Clientes"))
                {
                    show_clients_popup();
                }
                else
                {
                    show_access_denied();
                }
            }
        }
        else if(exit_status.reason == NEWT_EXIT_COMPONENT)
        {
            if(exit_status.u.co == btn_reports)
            {
                if(admin_action_allowed("Resultados"))
                {
                    show_results();
                }
                else
                {
                    show_access_denied();
                }
            }
            else if(exit_status.u.co == btn_logs)
            {
                if(admin_action_allowed("Logs"))
                {
                    show_logs_popup();
                }
                else
                {
                    show_access_denied();
                }
            }
            else if(exit_status.u.co == btn_clients)
            {
                if(admin_action_allowed("Clientes"))
                {
                    show_clients_popup();
                }
                else
                {
                    show_access_denied();
                }
            }
            else if(exit_status.u.co == btn_exit)
            {
                running = 0;
            }
        }
    }

    newtFormDestroy(form);
}
