#include <dirent.h>
#include <stdlib.h>
#include "cdl-utils.h"

bool is_valid_directory(char *dir)
{
    DIR *pDir;

    pDir = opendir(dir);
    return pDir != NULL;
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