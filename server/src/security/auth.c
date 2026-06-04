#include "auth.h"

#include <newt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int prompt_admin_password(
    char *password,
    size_t password_size,
    const char *action_name
)
{
    newtComponent form;
    newtComponent lbl_action;
    newtComponent lbl_password;
    newtComponent entry_password;
    newtComponent btn_ok;

    newtCenteredWindow(
        46,
        10,
        "Autenticacao"
    );

    lbl_action = newtLabel(
        2,
        2,
        action_name ? action_name : "Acao administrativa"
    );

    lbl_password = newtLabel(
        2,
        4,
        "Senha:"
    );

    entry_password = newtEntry(
        15,
        4,
        "",
        24,
        NULL,
        NEWT_FLAG_HIDDEN
    );

    btn_ok = newtButton(
        16,
        7,
        "OK"
    );

    form = newtForm(NULL, NULL, 0);

    newtFormAddComponents(
        form,
        lbl_action,
        lbl_password,
        entry_password,
        btn_ok,
        NULL
    );

    newtRunForm(form);

    snprintf(
        password,
        password_size,
        "%s",
        newtEntryGetValue(entry_password)
    );

    newtFormDestroy(form);
    newtPopWindow();

    return 1;
}

int admin_action_allowed(const char *action_name)
{
    const char *expected_password = getenv("VOTACAO_ADMIN_PASSWORD");

    if(!expected_password || expected_password[0] == '\0')
    {
        return 1;
    }

    char entered_password[128] = "";

    prompt_admin_password(
        entered_password,
        sizeof(entered_password),
        action_name
    );

    return strcmp(entered_password, expected_password) == 0;
}
