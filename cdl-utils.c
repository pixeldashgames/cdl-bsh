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

    char **ret = (char **)malloc(strLength * sizeof(char));

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

int readtoend(FILE *f, char *result)
{
    long file_size;
    char *buffer;

    // Determine the size of the file
    fseek(f, 0, SEEK_END);
    file_size = ftell(f);
    fseek(f, 0, SEEK_SET);

    // Allocate memory for the buffer
    buffer = malloc((file_size + 1) * sizeof(char));

    // Read the entire file into the buffer
    fread(buffer, file_size, 1, f);

    // Add a null terminator to the end of the buffer
    buffer[file_size] = '\0';

    return file_size;
}

char *clean_command(char *func)
{
    struct JaggedCharArray command_clean = splitstr(func, ' ');
    func = joinarr(command_clean, ' ', command_clean.count);
}

char *parse_function(char *func, struct JaggedCharArray operators)
{
    int function_len = strlen(func);

    for (int i = 0; i < operators.count; i++)
    {
        int index = findstr(func, operators.arr[i]);
        if (index == -1)
            continue;
        if (!strcmp(operators.arr[i], "if"))
        {
            char *result = malloc(MAX_COMMAND_LENGTH * sizeof(char));
            memset(result, 0, MAX_COMMAND_LENGTH);
            strcat(result, "if(");
            int then_index = findstr(func, "then");
            int if_then_size = (then_index - 1) - (index + 3);
            char *command1 = malloc(MAX_COMMAND_LENGTH * sizeof(char));
            memset(command1, 0, MAX_COMMAND_LENGTH);
            strncat(command1, func + index + 3, if_then_size);
            command1 = parse_function(command1, operators);
            strncat(result, command1, strlen(command1));
            strcat(result, ",");
            int else_index = findstr(func, "else");
            if (else_index != -1)
            {
                int then_else_size = else_index - then_index - 6;
                char *command2 = malloc(MAX_COMMAND_LENGTH * sizeof(char));
                memset(command2, 0, MAX_COMMAND_LENGTH);
                strncat(command2, func + then_index + 5, then_else_size);
                command2 = parse_function(command2, operators);
                strncat(result, command2, strlen(command2));
                free(command2);
            }
            else
                else_index = then_index;
            strcat(result, ",");

            int end_index = findstr(func, "end");
            int else_end_size = end_index - else_index - 6;
            char *command3 = malloc(MAX_COMMAND_LENGTH * sizeof(char));
            memset(command3, 0, MAX_COMMAND_LENGTH);
            strncat(command3, func + else_index + 5, else_end_size);
            command3 = parse_function(command3, operators);
            strncat(result, command3, strlen(command3));
            strcat(result, ")");
            free(command1);
            free(command3);
            return result;
        }
        char *left = malloc(MAX_COMMAND_LENGTH);
        memset(left, 0, MAX_COMMAND_LENGTH);
        int k = 0;
        int l = 0;
        while (k < index - 1)
            left[l++] = func[k++];
        k++;
        k += strlen(operators.arr[i]) + 1;
        left[l] = '\0';
        left = parse_function(left, operators);
        char *right = malloc(MAX_COMMAND_LENGTH);
        memset(right, 0, MAX_COMMAND_LENGTH);
        int ri = 0;
        while (k < function_len)
            right[ri++] = func[k++];
        k++;
        right[ri] = '\0';
        right = parse_function(right, operators);

        int result_len = strlen(right) + strlen(left) + strlen(operators.arr[i]) + 4;
        char *result = malloc(result_len * sizeof(char));
        memset(result, 0, result_len);
        int r = 0;
        for (int j = 0; j < strlen(operators.arr[i]); j++)
        {
            result[r++] = operators.arr[i][j];
        }
        result[r++] = '(';
        for (int j = 0; j < strlen(left); j++)
        {
            result[r++] = left[j];
        }
        result[r++] = ',';
        for (int j = 0; j < strlen(right); j++)
        {
            result[r++] = right[j];
        }
        result[r++] = ')';
        result[r] = '\0';
        free(left);
        free(right);
        return result;
    }
    return func;
}