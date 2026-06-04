#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#include "protocol.h"

static pthread_once_t receipt_seed_once = PTHREAD_ONCE_INIT;
static pthread_mutex_t receipt_mutex = PTHREAD_MUTEX_INITIALIZER;

static void seed_receipt_rng(void)
{
    srand((unsigned int)time(NULL));
}

void generate_receipt(char *buffer)
{
    pthread_once(&receipt_seed_once, seed_receipt_rng);

    pthread_mutex_lock(&receipt_mutex);

    snprintf(
        buffer,
        32,
        "REC-%d-%d",
        rand() % 100000,
        rand() % 100000
    );

    pthread_mutex_unlock(&receipt_mutex);
}
