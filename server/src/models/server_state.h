#ifndef SERVER_STATE_H
#define SERVER_STATE_H

#include <pthread.h>

#define MAX_LOGS 200

typedef struct
{
    int connected_clients;
    int total_votes;
    int active_threads;

    char logs[MAX_LOGS][256];
    int log_count;

} ServerState;

extern ServerState server_state;
extern pthread_mutex_t server_state_mutex;

void server_state_lock(void);
void server_state_unlock(void);
void add_log(const char *message);

#endif
