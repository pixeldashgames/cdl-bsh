#include <stdbool.h>

struct JaggedCharArray
{
    char **arr;
    int count;
};

bool is_valid_directory(char *dir);
int cntdigits(unsigned long long num);
struct JaggedCharArray splitstr(char *str, char sep);
char *joinarr(struct JaggedCharArray arr, char cep, int count);