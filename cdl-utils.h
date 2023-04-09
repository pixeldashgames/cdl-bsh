#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include "cdl-text-utils.h"
#include "parse_command.h"
#define MAX_COMMAND_LENGTH 8192
struct JaggedCharArray
{
    char **arr;
    int count;
};

bool is_valid_directory(char *dir);
int cntdigits(unsigned long long num);
struct JaggedCharArray splitstr(char *str, char sep);
char *joinarr(struct JaggedCharArray arr, char cep, int count);