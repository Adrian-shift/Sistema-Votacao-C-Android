#include <stdio.h>

#include <unistd.h>

#include <newt.h>

#include "ui/dashboard.h"

#include "database/database.h"

int main()
{
    if(!init_database())
    {
        printf("Erro ao iniciar banco\n");

        return 1;
    }

    newtInit();

    newtCls();

    start_dashboard();

    newtFinished();

    return 0;
}
