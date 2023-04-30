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

    char **ret = (char **)malloc(strLength * sizeof(char *));

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

char *dtryget(struct Dictionary dict, char *var, int *outidx)
{
    for (int i = 0; i < dict.count; i++)
        if (dict.pairs[i].key == var)
        {
            *outidx = i;
            return dict.pairs[i].value;
        }

    *outidx = -1;
    return NULL;
}

int dset(struct Dictionary *dict, char *var, char *value)
{
    int idx;
    dtryget(*dict, var, &idx);

    if (idx >= 0)
    {
        strcpy(dict->pairs[idx].value, value);
        return 0;
    }

    for (int i = 0; i < dict->count; i++)
    {
        if (!dict->pairs[i].hasValue)
        {
            dict->pairs[i].hasValue = true;
            strcpy(dict->pairs[i].key, var);
            strcpy(dict->pairs[i].value, value);
            return 0;
        }
    }

    return 1;
}

int dremove(struct Dictionary *dict, char *var)
{
    int idx;
    dtryget(*dict, var, &idx);

    if (idx >= 0)
    {
        dict->pairs[idx].hasValue = false;
        return 0;
    }

    return 1;
}

char *clean_command(char *func)
{
    struct JaggedCharArray command_clean = splitstr(func, ' ');
    for (int i = 0; i < command_clean.count; i++)
    {
        printf("%s \n", command_clean.arr[i]);
    }

    return joinarr(command_clean, ' ', command_clean.count);
}
bool is_binary(char *op)
{
    char binary_op[] = "; | && ||";
    struct JaggedCharArray operators = splitstr(binary_op, ' ');
    for (int i = 0; i < operators.count; i++)
    {
        if (strcmp(op, operators.arr[i]) == 0)
            return true;
    }
    return false;
}
bool is_bool(char *op)
{
    char bool_op[] = "true false";
    struct JaggedCharArray operators = splitstr(bool_op, ' ');
    for (int i = 0; i < operators.count; i++)
    {
        if (strcmp(op, operators.arr[i]) == 0)
            return true;
    }
    return false;
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
        if (is_binary(operators.arr[i]))
        {
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
        else
        {
            if (is_bool(func))
            {
                int result_len = strlen(func) + 3;
                char *result = malloc(result_len * sizeof(char));
                memset(result, 0, result_len);
                strcat(result, func);
                strcat(result, "()");
                result[result_len - 1] = '\0';
                return result;
            }
            else
            {
                int result_len = strlen(func) + 2;
                char *result = malloc(result_len * sizeof(char));
                memset(result, 0, result_len);
                struct JaggedCharArray parts = splitstr(func, ' ');
                strcat(result, parts.arr[0]);
                strcat(result, "(");
                for (int i = 1; i < parts.count; i++)
                {
                    strcat(result, parts.arr[i]);
                    if (i < parts.count - 1)
                        strcat(result, ",");
                }
                strcat(result, ")");
                result[result_len - 1] = '\0';
                return result;
            }
        }
    }
    return func;
}
int execute_pipe(char *command[], bool first, char *files[], int count)
{
    int fd_input = -1;
    int fd_output = -1;
    int status;
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    pid_t pid = fork();
    if (pid == 0)
    {
        if (first)
        {
            fd_output = open(files[count % 2], O_WRONLY | O_CREAT | O_TRUNC, mode);
            if (fd_output == -1)
            {
                perror("open");
                exit(EXIT_FAILURE);
            }
            close(STDOUT_FILENO);
            dup2(fd_output, STDOUT_FILENO);
            close(fd_output);
            status = execvp(command[0], command);
            perror("execvp");
            exit(EXIT_FAILURE);
        }
        else
        {
            fd_input = open(files[(count + 1) % 2], O_RDONLY, mode);
            fd_output = open(files[count % 2], O_WRONLY | O_CREAT | O_TRUNC, mode);
            close(STDOUT_FILENO);
            dup2(fd_input, STDIN_FILENO);
            dup2(fd_output, STDOUT_FILENO);
            close(fd_input);
            close(fd_output);
            status = execvp(command[0], command);
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        waitpid(pid, &status, 0);
        close(fd_input);
        close(fd_output);
        remove(files[(count + 1) % 2]);
    }
    return 0;
}
void copy_string_array(char **src, char **dest, int size)
{
    for (int i = 0; i < size; i++)
    {
        dest[i] = malloc((strlen(src[i]) + 1) * sizeof(char));
        memset(dest[i], 0, strlen(src[i] + 1));
        strcpy(dest[i], src[i]);
    }
}
bool is_command(char *function)
{
    int len = strlen(function);
    for (int i = 0; i < len; i++)
    {
        if (function[i] == '(')
            return false;
    }
    return true;
}
bool First = true;
void main_execute(char *function, int count, char *files[])
{
    if (is_command(function))
    {
        struct JaggedCharArray split_func = splitstr(function, ' ');
        char **new_func = malloc((split_func.count + 1) * sizeof(char *));
        copy_string_array(split_func.arr, new_func, split_func.count);
        new_func[split_func.count] = malloc(sizeof(NULL));
        new_func[split_func.count] = NULL;
        execute_pipe(new_func, First, files, count);
        First = false;
        free(new_func);
        return;
    }

    int parenthesis_init = findstr(function, "(");
    char *op = malloc((parenthesis_init + 1) * sizeof(char));
    memset(op, 0, parenthesis_init + 1);
    strncpy(op, function, parenthesis_init);
    int op_len = strlen(op);
    if (op_len == 1 && op[0] == '|')
    {
        execute(function, count, files);
    }
}
void execute(char *function, int count, char *files[])
{
    int parenthesis_init = findstr(function, "(");
    int comma_index = -1;
    int parenth_count = 0;
    int len = strlen(function);
    for (int i = parenthesis_init + 1; i < len; i++)
    {
        if (function[i] == ',' && parenth_count == 0)
        {
            comma_index = i;
            break;
        }
        if (function[i] == '(')
            parenth_count++;
        if (function[i] == ')')
            parenth_count--;
    }
    int left_size = comma_index - parenthesis_init - 1;
    char *left = malloc((left_size + 1) * sizeof(char));
    memset(left, 0, (left_size + 1) * sizeof(char));
    memcpy(left, function + parenthesis_init + 1, (left_size) * sizeof(char));
    int right_size = (len - 1) - comma_index - 1;
    char *right = malloc((right_size + 1) * sizeof(char));
    memset(right, 0, (right_size + 1) * sizeof(char));
    memcpy(right, function + comma_index + 1, right_size * sizeof(char));
    main_execute(left, count, files);
    count++;
    main_execute(right, count, files);
    free(right);
    free(left);
    return;
}