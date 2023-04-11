#include "cdl-utils.c"

struct JaggedCharArray operators;
void parse_flow(char **func);
int main()
{
    char op[] = "| > >> <";
    char *command = malloc(MAX_COMMAND_LENGTH * sizeof(char));
    strcat(command, "command1 < file1 | command2");
    operators = splitstr(op, ' ');
    parse_flow(&command);
    printf(command);
}
void parse_flow(char **func)
{
    int function_len = strlen(*func);

    for (int i = 0; i < operators.count; i++)
    {
        int index = search_pattern(*func, operators.arr[i]);
        if (index == -1)
            continue;
        char *left = malloc(MAX_COMMAND_LENGTH);
        memcpy(left, *func, index);
        struct JaggedCharArray l = splitstr(left, ' ');
        left = joinarr(l, ' ', l.count);
        parse_flow(&left);

        char *right = malloc(MAX_COMMAND_LENGTH);
        memcpy(left, *func + index + 1, function_len - index);
        struct JaggedCharArray r = splitstr(right, ' ');
        right = joinarr(r, ' ', r.count);
        parse_flow(&right);

        int result_len = strlen(right) + strlen(left) + strlen(operators.arr[i]) + 4;
        char *result = malloc(result_len * sizeof(char));

        strcat(result, operators.arr[i]);
        strcat(result, "(");
        strcat(result, left);
        strcat(result, ",");
        strcat(result, right);
        strcat(result, ")");
        result[result_len - 1] = '\0';

        for (int j = 0; j < result_len; j++)
        {
            (*func)[j] = result[j];
        }
    }
}
