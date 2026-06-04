#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "server_state.h"

ServerState server_state = {
    .connected_clients = 0,
    .total_votes = 0,
    .active_threads = 1,
    .log_count = 0
};

pthread_mutex_t server_state_mutex = PTHREAD_MUTEX_INITIALIZER;

void server_state_lock(void)
{
    pthread_mutex_lock(&server_state_mutex);
}

void server_state_unlock(void)
{
    pthread_mutex_unlock(&server_state_mutex);
}

void add_log(const char *message)
{
    server_state_lock();

    if(server_state.log_count >= MAX_LOGS)
    {
        for(int i = 1; i < MAX_LOGS; i++)
        {
            memmove(
                server_state.logs[i - 1],
                server_state.logs[i],
                sizeof(server_state.logs[i])
            );
        }

        server_state.log_count--;
    }

    snprintf(
        server_state.logs[server_state.log_count],
        256,
        "%s",
        message
    );

    server_state.log_count++;

    server_state_unlock();
}
