#include "cdl-utils.h"
void execute(char *function, int **count, char files[], bool is_pipe);
int main()
{
    char command[] = "ls ./ | gred cdl";
    char op[] = "|";
    char *parse_command = malloc(MAX_COMMAND_LENGTH * sizeof(char));
    parse_command = parse_function(command, splitstr(op, ' '));
    printf("%s\n", parse_command);
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
bool FIRST = true;
void execute(char *function, int **count, char files[], bool is_pipe)
{
    if (is_pipe && is_command(function))
    {
        // TODO: append to every command the NULL, getting: char *command[] = {"command","param1",...,"paramN",Null};
        // PD: char *command[] and char **command are the same

        size_t funlen = strlen(function);
        char *newfunc = malloc((funlen + 3) * sizeof(char));
        sprintf(newfunc, "%s $", function);

        struct JaggedCharArray split_function = splitstr(newfunc, ' ');

        *split_function.arr[split_function.count - 1] = NULL;

        execute_pipe(split_function.arr, FIRST, files, (*count)++);
        return;
    }
    int comma_index = -1;
    int parenthesis_init = findstr(function, '(');
    char *op = malloc((parenthesis_init + 1) * sizeof(char));
    memcpy(op, function, parenthesis_init * sizeof(char));
    op[parenthesis_init + 1] = '\n';
    if (strcmp(op, "|") == 0)
    {
        int parenth_count = 0;
        int len = strlen(function);
        for (int i = parenthesis_init + 1; i < len; i++)
        {
            if (parenth_count == 0 && i != (parenthesis_init + 1))
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
        memcpy(left, function + parenthesis_init + 1, (left_size) * sizeof(char));
        left[left_size] = '\n';

        int right_size = len - comma_index - 1;
        char *right = malloc((right_size + 1) * sizeof(char));
        memcpy(right, function + comma_index + 1, right_size * sizeof(char));
        right[right_size] = '\n';

        execute(left, &count, files, true);
        FIRST = false;
        execute(right, &count, files, true);
        free(right);
        free(left);
    }
    return;
}