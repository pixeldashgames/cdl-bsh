#include <dirent.h>
#include <stdlib.h>
#include <ctype.h>
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

int indexOf(unsigned long long elem, unsigned long long *array, int cnt)
{
    int i;
    for (i = 0; i < cnt; i++)
    {
        if (array[i] == elem)
            return i;
    }
    return -1;
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
        if (len == 0)
        {
            tokenPointer = i + 1;
            continue;
        }

        size_t size = len * sizeof(char);
        ret[count] = malloc(size + sizeof(char));

        memcpy(ret[count], str + tokenPointer, size);
        ret[count][len] = '\0';

        count++;
        tokenPointer = i + 1;
    }

    if (strLength > tokenPointer)
    {
        int len = strLength - tokenPointer;
        size_t size = len * sizeof(char);

        ret[count] = malloc(size + sizeof(char));

        memcpy(ret[count], str + tokenPointer, size);
        ret[count][len] = '\0';
        count++;
    }

    size_t size = count * sizeof(char *);
    char **result = malloc(size);
    memcpy(result, ret, size);

    free(ret);

    struct JaggedCharArray jaggedArr = {result, count};
    return jaggedArr;
}

char *joinarr(struct JaggedCharArray arr, char sep, int count)
{
    if (count == 0)
        return "";

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
        strcpy(pret, arr.arr[i]);
        len = strlen(pret);
        pret[len] = sep;
        pret += len + 1;
    }
    strcpy(pret, arr.arr[count - 1]);
    len = strlen(pret);
    pret[len] = '\0';

    return ret;
}

// returns the index of the first time tok is matched in str, left to right.
int findstr(char *str, char *tok)
{
    size_t len = strlen(str);
    size_t toklen = strlen(tok);
    size_t q = 0; // matched chars so far

    for (int i = 0; i < len; i++)
    {
        if (q > 0 && str[i] != tok[q])
            q = 0; // this works since in the use cases for this function tokens are space separated and
                   // don't start with a space, so there is no need for a prefix function.
        if (str[i] == tok[q])
            q++;
        if (q == toklen)
            return i - q + 1;
    }

    return -1;
}

// extracts an integer from str that starts in startpos
int extractint(char *str, int startpos, int *len)
{
    size_t maxlen = strlen(str) - startpos;
    char *num = malloc(maxlen * sizeof(char) + sizeof(char));

    int count = 0;

    while (count < maxlen && (isdigit(str[startpos + count]) || str[startpos + count] == '-'))
    {
        num[count] = str[startpos + count];
        count++;
    }

    num[count] = '\0';

    *len = count;

    return atoi(num);
}

void replacestr(char *source, char *target, int start, int len)
{
    size_t srclen = strlen(source);
    size_t tgtlen = strlen(target);

    size_t extralen = srclen - start - len;

    if (extralen == 0)
    {
        memcpy(source + start, target, tgtlen * sizeof(char));
        source[start + tgtlen] = '\0';

        return;
    }

    char *buffer = malloc(extralen * sizeof(char) + sizeof(char));
    strcpy(buffer, source + start + len);

    strcpy(source + start, target);
    strcpy(source + start + tgtlen, buffer);

    free(buffer);
}
