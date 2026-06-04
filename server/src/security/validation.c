#include "validation.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

int is_valid_voter_id(const char *voter_id)
{
    size_t len;

    if(!voter_id)
    {
        return 0;
    }

    len = strnlen(voter_id, MAX_VOTER_ID_LEN + 1);
    if(len == 0 || len > MAX_VOTER_ID_LEN)
    {
        return 0;
    }

    for(size_t i = 0; i < len; i++)
    {
        if(!isalnum((unsigned char)voter_id[i]))
        {
            return 0;
        }
    }

    return 1;
}

int is_valid_candidate(const char *candidate)
{
    if(!candidate)
    {
        return 0;
    }

    return
        strcmp(candidate, "candidatoA") == 0 ||
        strcmp(candidate, "candidatoB") == 0 ||
        strcmp(candidate, "candidatoC") == 0;
}

int parse_vote_message(
    const char *buffer,
    char *voter_id,
    size_t voter_size,
    char *candidate,
    size_t candidate_size
)
{
    char extra[2];
    char format[64];
    size_t voter_limit;
    size_t candidate_limit;

    if(!buffer || !voter_id || !candidate)
    {
        return 0;
    }

    if(voter_size < 2 || candidate_size < 2)
    {
        return 0;
    }

    voter_limit = voter_size - 1;
    if(voter_limit > MAX_VOTER_ID_LEN)
    {
        voter_limit = MAX_VOTER_ID_LEN;
    }

    candidate_limit = candidate_size - 1;
    if(candidate_limit > MAX_CANDIDATE_ID_LEN)
    {
        candidate_limit = MAX_CANDIDATE_ID_LEN;
    }

    snprintf(
        format,
        sizeof(format),
        "%%%zus %%%zus %%1s",
        voter_limit,
        candidate_limit
    );

    if(sscanf(buffer, format, voter_id, candidate, extra) != 2)
    {
        return 0;
    }

    if(!is_valid_voter_id(voter_id))
    {
        return 0;
    }

    if(!is_valid_candidate(candidate))
    {
        return 0;
    }

    return 1;
}
