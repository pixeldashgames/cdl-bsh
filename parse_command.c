#include "cdl-utils.c"
// TODO: fix error using if commands in linux
void parse_flow(char **func, struct JaggedCharArray operators);
int main()
{
    char op[] = "; | > >> < if";
    char *p_op = op;
    char *command = malloc(MAX_COMMAND_LENGTH * sizeof(char));
    memset(command, 0, strlen(command));
    strcat(command, "bool < file1 | com2");
    struct JaggedCharArray operators = splitstr(p_op, ' ');
    struct JaggedCharArray command_clean = splitstr(command, ' ');
    command = joinarr(command_clean, ' ', command_clean.count);
    parse_flow(&command, operators);
    printf("%s \n", command);
    free(command);
}
void parse_flow(char **func, struct JaggedCharArray operators)
{
    int function_len = strlen(*func);

    for (int i = 0; i < operators.count; i++)
    {
        int index = findstr(*func, operators.arr[i]);
        if (index == -1)
            continue;
        if (strcmp(operators.arr[i], "if") == 0)
        {
            // Maybe copy this in a different method
            int result_size = strlen(*func) + 1;
            char *result = malloc(result_size * sizeof(char));
            memset(result, 0, result_size);
            strcat(result, "if(");

            int then_index = findstr(*func, "then");
            int if_then_size = then_index - index - 3;
            // char *if_then = malloc(if_then_size * sizeof(char));
            // memset(if_then, 0, if_then_size);
            strncat(result, *func + index + 2 + 1, if_then_size - 1);
            strcat(result, ",");

            int else_index = findstr(*func, "else");
            if (else_index != -1)
            {
                int then_else_size = else_index - then_index - 6;
                // char *then_else = malloc(then_else_size * sizeof(char));
                // memset(then_else, 0, then_else_size);
                strncat(result, *func + then_index + 4 + 1, then_else_size);
            }
            else
                else_index = then_index;
            strcat(result, ",");

            int end_index = findstr(*func, "end");
            int else_end_size = end_index - else_index - 6;
            // char *else_end = malloc(else_end_size * sizeof(char));
            // memset(else_end, 0, else_end_size);
            strncat(result, *func + else_index + 4 + 1, else_end_size);
            strcat(result, ")");
            int len = strlen(result);
            memset(*func, 0, strlen(*func));
            for (int j = 0; j < len; j++)
            {
                (*func)[j] = result[j];
            }
            free(result);
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

        memset(*func, 0, strlen(*func));
        for (int j = 0; j < result_len; j++)
        {
            (*func)[j] = result[j];
        }
        free(left);
        free(right);
        free(result);
        break;
    }
}
