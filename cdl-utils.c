#include <dirent.h>
#include <stdlib.h>
#include "cdl-utils.h"

bool is_valid_directory(char *dir)
{
    DIR *pDir;

    pDir = opendir(dir);
    if (pDir == NULL)
        return false;

    closedir(pDir);

    return true;
}

int cntdigits(unsigned long long num)
{
    int count = 0;
    do
    {
        num /= 10;
        count++;
    } while (num > 0);

    return count;
}

struct JaggedCharArray splitstr(char *str, char sep)
{
    int strLength = strlen(str);
    int count = 0;
    int i;
    int tokenPointer = 0;

    char **ret = malloc(strLength * sizeof(char *));

    for (i = 0; i < strLength; i++)
    {

        if (str[i] != sep)
            continue;

        int len = i - tokenPointer;
        size_t size = len * sizeof(char);
        if (len == 0)
        {
            tokenPointer = i + 1;
            continue;
        }

        ret[count] = malloc(size);

        memcpy(ret[count], str + tokenPointer, size);

        count++;
        tokenPointer = i + 1;
    }

    if (strLength > tokenPointer)
    {
        int len = strLength - tokenPointer;
        size_t size = len * sizeof(char);

        ret[count] = malloc(size);

        memcpy(ret[count], str + tokenPointer, size);
        count++;
    }
    size_t size = count * sizeof(char *);
    char **result = malloc(size);
    memcpy(ret, result, size);

    free(ret);

    struct JaggedCharArray jaggedArr = {result, count};
    return jaggedArr;
}

char *joinarr(struct JaggedCharArray arr, char sep, int count)
{
    int i;
    int retLen = 0;
    for (i = 0; i < count; i++)
    {
        retLen += strlen(arr.arr[i]) + 1;
    }

    int len;
    char *ret = malloc(retLen * sizeof(char));
    char *pret = ret;
    for (i = 0; i < count - 1; i++)
    {
        pret = *arr.arr[i];
        len = strlen(pret);
        pret[len] = sep;
        pret += len + 1;
    }
    pret[-1] = '\0';

    return ret;
}
