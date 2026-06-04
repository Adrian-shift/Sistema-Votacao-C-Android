#include "secure_log.h"

#include <stdio.h>

#include "../models/server_state.h"

void security_log_event(const char *category, const char *detail)
{
    char message[256];

    if(!category)
    {
        category = "GERAL";
    }

    if(!detail)
    {
        detail = "";
    }

    snprintf(
        message,
        sizeof(message),
        "[SEC][%s] %s",
        category,
        detail
    );

    add_log(message);
}
