#include <newt.h>
#include <stdlib.h>
#include <string.h>

#include "login.h"

void show_login(
    char *ip,
    int *port,
    char *voter_id
)
{
    newtComponent form;

    newtComponent ip_entry;

    newtComponent port_entry;

    newtComponent voter_entry;

    newtComponent btn_ok;

    char port_str[16];

    strcpy(ip, "127.0.0.1");

    strcpy(port_str, "8080");

    newtCenteredWindow(
        50,
        15,
        "Login Eleitor"
    );

    newtComponent lbl_ip =
        newtLabel(2, 2, "IP:");

    ip_entry =
        newtEntry(
            15,
            2,
            ip,
            20,
            NULL,
            0
        );

    newtComponent lbl_port =
        newtLabel(2, 5, "Porta:");

    port_entry =
        newtEntry(
            15,
            5,
            port_str,
            10,
            NULL,
            0
        );

    newtComponent lbl_voter =
        newtLabel(2, 8, "Eleitor ID:");

	voter_entry =
		newtEntry(
			15,
			8,
			"",
			20,
			NULL,
			0
		);

    btn_ok =
        newtButton(
            18,
            11,
            "Conectar"
        );

    form = newtForm(NULL, NULL, 0);

    newtFormAddComponents(
        form,

        lbl_ip,
        ip_entry,

        lbl_port,
        port_entry,

        lbl_voter,
        voter_entry,

        btn_ok,

        NULL
    );

    newtRunForm(form);

    strcpy(ip, newtEntryGetValue(ip_entry));

    *port = atoi(
        newtEntryGetValue(port_entry)
    );

    strcpy(
        voter_id,
        newtEntryGetValue(voter_entry)
    );

    newtFormDestroy(form);
}
