#include <stdbool.h>

struct JaggedCharArray
{
    char **arr;
    int count;
};

bool is_valid_directory(char *dir);
struct JaggedCharArray splitstr(char *str, char sep);