#ifndef DATABASE_H
#define DATABASE_H

#include <sqlite3.h>

extern sqlite3 *db;

int init_database();

int voter_exists(const char *voter_id);

int has_voted(const char *voter_id);

int count_votes();

int get_vote_results(
    char results[][128],
    int counts[],
    int max_results
);

int save_vote(
    const char *voter_id,
    const char *candidate,
    const char *receipt
);

#endif
