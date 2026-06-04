#ifndef VALIDATION_H
#define VALIDATION_H

#include <stddef.h>

#define MAX_VOTER_ID_LEN 63
#define MAX_CANDIDATE_ID_LEN 63

int parse_vote_message(
    const char *buffer,
    char *voter_id,
    size_t voter_size,
    char *candidate,
    size_t candidate_size
);

int is_valid_voter_id(const char *voter_id);
int is_valid_candidate(const char *candidate);

#endif
