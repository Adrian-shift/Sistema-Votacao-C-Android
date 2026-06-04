#include <newt.h>
#include <stdio.h>
#include <string.h>

#include "voting.h"

void show_voting_menu(
    char *candidate
)
{
    newtComponent form;
    newtComponent lbl_title;
    newtComponent lbl_help;
    newtComponent lbl_hint;

    newtComponent listbox;

    newtComponent btn_vote;

    newtCenteredWindow(
        50,
        18,
        "Escolha seu candidato"
    );

    lbl_title =
        newtLabel(
            2,
            1,
            "SELECAO DE CANDIDATO"
        );

    lbl_help =
        newtLabel(
            2,
            2,
            "Use as setas para navegar."
        );

    lbl_hint =
        newtLabel(
            2,
            3,
            "Enter confirma a escolha."
        );

    listbox =
        newtListbox(
            5,
            5,
            6,
            NEWT_FLAG_RETURNEXIT
        );

    newtListboxAppendEntry(
        listbox,
        "10 - Candidato A",
        "candidatoA"
    );

    newtListboxAppendEntry(
        listbox,
        "20 - Candidato B",
        "candidatoB"
    );

    newtListboxAppendEntry(
        listbox,
        "30 - Candidato C",
        "candidatoC"
    );

    newtListboxSetCurrent(
        listbox,
        0
    );

    btn_vote =
        newtButton(
            16,
            13,
            "Confirmar Voto"
        );

    form = newtForm(NULL, NULL, 0);

    newtFormAddComponents(
        form,
        lbl_title,
        lbl_help,
        lbl_hint,
        listbox,
        btn_vote,
        NULL
    );

    newtRunForm(form);

    char *selected =
        (char*)newtListboxGetCurrent(listbox);

    if(selected == NULL)
    {
        selected = "candidatoA";
    }

    strcpy(candidate, selected);

    newtFormDestroy(form);
}
