#include <newt.h>

#include "receipt.h"

void show_receipt(
    char *response
)
{
    newtWinMessage(
        "Resultado",
        "OK",
        response
    );
}
