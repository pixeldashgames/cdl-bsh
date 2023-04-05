#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include "cdl-utils.h"

#define MAX_COMMAND_LENGTH 8192

int main()
{
    char *cmd = malloc(MAX_COMMAND_LENGTH * sizeof(char));
    char *parsed_cmd = malloc(MAX_COMMAND_LENGTH * sizeof(char));
    int len = 0;
    for (size_t i = 0; i < strlen(cmd); i++)
    {
        if (cmd[i] == '|')
        {
        }
    }
}