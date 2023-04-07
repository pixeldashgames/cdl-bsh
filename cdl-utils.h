#include <stdbool.h>
#define MAX_COMMAND_LENGTH 8192
struct JaggedCharArray
{
    char **arr;
    int count;
};

bool is_valid_directory(char *dir);
struct JaggedCharArray splitstr(char *str, char sep);