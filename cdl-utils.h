#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include "cdl-text-utils.h"
#include <signal.h>
#define MAX_COMMAND_LENGTH 8192


struct JaggedCharArray
{
    char **arr;
    int count;
};

struct KeyValuePair
{
    char *key;
    char *value;
    bool hasValue;
};

struct Dictionary
{
    struct KeyValuePair *pairs;
    int count;
};

char *dtryget(struct Dictionary dict, char *var, int *outidx);
int dset(struct Dictionary *dict, char *var, char *value);
int dremove(struct Dictionary *dict, char *var);
bool is_valid_directory(char *dir);
int cntdigits(unsigned long long num);
struct JaggedCharArray splitstr(char *str, char sep);
char *joinarr(struct JaggedCharArray arr, char cep, int count);
int indexOf(unsigned long long elem, unsigned long long *array, int cnt);
int findstr(char *str, char *tok);
int extractint(char *str, int startpos, int *len);
// replaces source[start..len] with target
void replacestr(char *source, char *target, int start, int len);
int readtoend(FILE *f, char *result);
char *clean_command(char *func);
char *parse_function(char *func, struct JaggedCharArray operators);
