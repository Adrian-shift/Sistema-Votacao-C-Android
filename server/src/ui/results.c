#include <newt.h>
#include <stdio.h>
#include <string.h>

#include "../database/database.h"
#include "results.h"

static const char* format_candidate(const char *candidate)
{
    if(candidate == NULL)
    {
        return "desconhecido";
    }

    if(strcmp(candidate, "candidatoA") == 0)
    {
        return "10 - Candidato A";
    }

    if(strcmp(candidate, "candidatoB") == 0)
    {
        return "20 - Candidato B";
    }

    if(strcmp(candidate, "candidatoC") == 0)
    {
        return "30 - Candidato C";
    }

    return candidate;
}

void show_results()
{
    char candidates[16][128];
    int counts[16];
    char text[2048] = "";
    size_t used = 0;

    int total_results = get_vote_results(
        candidates,
        counts,
        16
    );

    char header[256];

    snprintf(
        header,
        sizeof(header),
        "Resultados da votacao\n\nTotal de votos: %d\n\n",
        count_votes()
    );

    used = (size_t)snprintf(text, sizeof(text), "%s", header);
    if(used >= sizeof(text))
    {
        used = sizeof(text) - 1;
    }

    if(total_results == 0)
    {
        snprintf(
            text + used,
            sizeof(text) - used,
            "Nenhum voto registrado ainda.\n"
        );
    }
    else
    {
        for(int i = 0; i < total_results; i++)
        {
            char line[256];

            snprintf(
                line,
                sizeof(line),
                "%s: %d\n",
                format_candidate(candidates[i]),
                counts[i]
            );

            int written = snprintf(
                text + used,
                sizeof(text) - used,
                "%s",
                line
            );

            if(written < 0 || (size_t)written >= sizeof(text) - used)
            {
                break;
            }

            used += (size_t)written;
        }
    }

    newtCenteredWindow(
        52,
        16,
        "Resultados"
    );

    newtComponent form;
    newtComponent box;
    newtComponent btn_close;

    box = newtTextbox(
        2,
        2,
        48,
        9,
        NEWT_FLAG_BORDER | NEWT_FLAG_SCROLL
    );

    btn_close = newtButton(
        20,
        12,
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
