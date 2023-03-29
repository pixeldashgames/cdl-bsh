#include <windows.h>
#include "text_utils.h"

WORD toWindowsAttribute(char *token, WORD savedAttributes, WORD currentAttributes)
{
    if (strcmp(token, RED))
        return WIN_RED;
    if (strcmp(token, BLUE))
        return WIN_BLUE;
    if (strcmp(token, GREEN))
        return WIN_GREEN;
    if (strcmp(token, YELLOW))
        return WIN_YELLOW;
    if (strcmp(token, CYAN))
        return WIN_CYAN;
    if (strcmp(token, BOLD))
        return WIN_BOLD;
    if (strcmp(token, COLOR_RESET))
        return savedAttributes;
    if (strcmp(token, BOLD_RESET))
        return currentAttributes & ~WIN_BOLD;
        
    return NULL;
}