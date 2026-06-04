#include "rate_limit.h"

#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <time.h>

#define MAX_RATE_LIMIT_ENTRIES 32
#define RATE_LIMIT_WINDOW_SECONDS 60
#define RATE_LIMIT_MAX_REQUESTS 5

typedef struct
{
    int active;
    char ip[INET_ADDRSTRLEN];
    time_t timestamps[RATE_LIMIT_MAX_REQUESTS];
    int count;
    time_t last_seen;
} rate_limit_entry_t;

static rate_limit_entry_t entries[MAX_RATE_LIMIT_ENTRIES];
static pthread_mutex_t rate_limit_mutex = PTHREAD_MUTEX_INITIALIZER;

static int find_entry_index(const char *ip)
{
    int free_slot = -1;
    int oldest_slot = 0;
    time_t oldest_seen = 0;

    for(int i = 0; i < MAX_RATE_LIMIT_ENTRIES; i++)
    {
        if(entries[i].active && strcmp(entries[i].ip, ip) == 0)
        {
            return i;
        }

        if(!entries[i].active && free_slot == -1)
        {
            free_slot = i;
        }

        if(entries[i].active && (oldest_seen == 0 || entries[i].last_seen < oldest_seen))
        {
            oldest_seen = entries[i].last_seen;
            oldest_slot = i;
        }
    }

    if(free_slot != -1)
    {
        return free_slot;
    }

    return oldest_slot;
}

static void prune_entry(rate_limit_entry_t *entry, time_t now)
{
    int write_index = 0;

    for(int i = 0; i < entry->count; i++)
    {
        if(now - entry->timestamps[i] <= RATE_LIMIT_WINDOW_SECONDS)
        {
            entry->timestamps[write_index++] = entry->timestamps[i];
        }
    }

    entry->count = write_index;
}

int rate_limit_allow(const char *client_ip)
{
    time_t now = time(NULL);

    if(!client_ip || client_ip[0] == '\0')
    {
        return 0;
    }

    pthread_mutex_lock(&rate_limit_mutex);

    int index = find_entry_index(client_ip);
    rate_limit_entry_t *entry = &entries[index];

    if(!entry->active || strcmp(entry->ip, client_ip) != 0)
    {
        memset(entry, 0, sizeof(*entry));
        entry->active = 1;
        snprintf(entry->ip, sizeof(entry->ip), "%s", client_ip);
    }

    prune_entry(entry, now);

    if(entry->count >= RATE_LIMIT_MAX_REQUESTS)
    {
        entry->last_seen = now;
        pthread_mutex_unlock(&rate_limit_mutex);
        return 0;
    }

    entry->timestamps[entry->count++] = now;
    entry->last_seen = now;

    pthread_mutex_unlock(&rate_limit_mutex);
    return 1;
}
