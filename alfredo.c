#include "cdl-utils.h"
void execute(char *function, int *count);
// char *files[] = {"/tmp/cdl-temp0.txt", "/tmp/cdl-temp1.txt"};
char *files[] = {"cdl-temp0.txt", "cdl-temp1.txt"};
int main()
{
    char command[] = "ls ./ | gred cdl";
    char op[] = "|";
    char *parse_command = malloc(MAX_COMMAND_LENGTH * sizeof(char));
    parse_command = parse_function(command, splitstr(op, ' '));
    int count = 0;
    execute(parse_command, &count);
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
void execute(char *function, int *count)
{
    if (is_command(function))
    {
        struct JaggedCharArray split_func = splitstr(function, ' ');
        char **new_func = malloc((split_func.count + 1) * sizeof(char *));
        copy_string_array(split_func.arr, new_func, split_func.count);
        new_func[split_func.count] = malloc(sizeof(NULL));
        new_func[split_func.count] = NULL;
        printf("%i\n", *count);
        execute_pipe(new_func, (count == 0) ? true : false, files, *count);
        dup2(0, STDIN_FILENO);
        dup2(1, STDOUT_FILENO);
        (*count)++;
        free(new_func);
        return;
    }

    int comma_index = -1;
    int parenthesis_init = findstr(function, "(");
    char *op = malloc((parenthesis_init + 1) * sizeof(char));
    memset(op, 0, parenthesis_init + 1);
    strncpy(op, function, parenthesis_init);
    int op_len = strlen(op);
    if (op_len == 1 && op[0] == '|')
    {
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
        execute(left, &(*count));
        execute(right, &(*count));
        free(right);
        free(left);
    }
    return;
}