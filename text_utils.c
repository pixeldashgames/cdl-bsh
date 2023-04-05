#ifdef _WIN32
#include <windows.h>
#include <stdio.h>
#include <stdbool.h>
#include "text_utils.h"

struct WindowsAttribute toWindowsAttribute(char *token, WORD savedAttributes, WORD currentAttributes)
{
    PWORD result = malloc(sizeof(WORD));
    bool valid = false;

    bool isBold = currentAttributes * WIN_BOLD != 0;

    if (strcmp(token, RED) == 0)
    {
        valid = true;
        *result = WIN_RED | (isBold ? WIN_BOLD : 0);
    }
    else if (strcmp(token, BLUE) == 0)
    {
        valid = true;
        *result = WIN_BLUE | (isBold ? WIN_BOLD : 0);
    }
    else if (strcmp(token, GREEN) == 0)
    {
        valid = true;
        *result = WIN_GREEN | (isBold ? WIN_BOLD : 0);
    }
    else if (strcmp(token, YELLOW) == 0)
    {
        valid = true;
        *result = WIN_YELLOW | (isBold ? WIN_BOLD : 0);
    }
    else if (strcmp(token, CYAN) == 0)
    {
        valid = true;
        *result = WIN_CYAN | (isBold ? WIN_BOLD : 0);
    }
    else if (strcmp(token, BOLD) == 0)
    {
        valid = true;
        *result = currentAttributes | WIN_BOLD;
    }
    else if (strcmp(token, COLOR_RESET) == 0)
    {
        valid = true;
        *result = savedAttributes | (isBold ? WIN_BOLD : 0);
    }
    else if (strcmp(token, BOLD_RESET) == 0)
    {
        valid = true;
        *result = currentAttributes & ~WIN_BOLD;
    }

    struct WindowsAttribute ret = {result, valid};
    return ret;
}
#endif