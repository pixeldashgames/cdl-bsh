#include "cdl-utils.h"
void parse_flow(char **func, struct JaggedCharArray operators);
int main()
{
    char op[] = "; | > >> < if";
    char *p_op = op;
    char *command = malloc(MAX_COMMAND_LENGTH * sizeof(char));
    memset(command, 0, strlen(command));
    strcat(command, "command1 < file1 | command2");
    struct JaggedCharArray operators = splitstr(p_op, ' ');
    struct JaggedCharArray command_clean = splitstr(command, ' ');
    command = joinarr(command_clean, ' ', strlen(command));
    parse_flow(&command, operators);
    printf("%s", command);
}
void parse_flow(char **func, struct JaggedCharArray operators)
{
    int function_len = strlen(*func);

    for (int i = 0; i < operators.count; i++)
    {
        int index = search_pattern(*func, operators.arr[i]);
        if (index == -1)
            continue;
        if (strcmp(operators.arr[i], "if"))
        {
            // Maybe copy this in a different method
            int result_size = strlen(*func) + 1;
            char *result = malloc(result_size * sizeof(char));
            memset(result, 0, result_size);
            strcat(result, "if(");
            int then_index = search_pattern(*func, "else");
            // TODO: finish assign fragments bewteen if then else end statments so it would be like if(if_then,then_else,else_end)
            break;
        }
        char *left = malloc(MAX_COMMAND_LENGTH);
        memset(left, 0, MAX_COMMAND_LENGTH);
        memcpy(left, *func, index);
        struct JaggedCharArray l = splitstr(left, ' ');
        left = joinarr(l, ' ', l.count);
        parse_flow(&left, operators);

        char *right = malloc(MAX_COMMAND_LENGTH);
        memset(right, 0, MAX_COMMAND_LENGTH);
        memcpy(right, *func + index + 1, function_len - index);
        struct JaggedCharArray r = splitstr(right, ' ');
        right = joinarr(r, ' ', r.count);
        parse_flow(&right, operators);

        int result_len = strlen(right) + strlen(left) + strlen(operators.arr[i]) + 4;
        char *result = malloc(result_len * sizeof(char));
        memset(result, 0, result_len);

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
        break;
    }
}
