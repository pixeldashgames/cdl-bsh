#include "cdl-utils.c"

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
            bool else_founded = true;
            if (else_index != -1)
            {
                int then_else_size = else_index - then_index - 6;
                char *command2 = malloc(MAX_COMMAND_LENGTH * sizeof(char));
                memset(command2, 0, MAX_COMMAND_LENGTH);
                strncat(command2, func + then_index + 5, then_else_size);
                command2 = parse_function(command2, operators);
                strncat(result, command2, strlen(command2));
                free(command2);
                strcat(result, ",");
            }
            else
            {
                else_index = then_index;
                else_founded = false;
            }
            int end_index = findstr(func, "end");
            int else_end_size = end_index - else_index - 6;
            char *command3 = malloc(MAX_COMMAND_LENGTH * sizeof(char));
            memset(command3, 0, MAX_COMMAND_LENGTH);
            strncat(command3, func + else_index + 5, else_end_size);
            command3 = parse_function(command3, operators);
            strncat(result, command3, strlen(command3));
            if (!else_founded)
                strcat(result, ",");
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
char *files[] = {"cdl-temp0.txt", "cdl-temp1.txt"};
char help[] = "Members: Leonardo Amaro Rodriguez and Alfredo Montero Lopez\n"
              "\nFeatures:\nbasics\nmulti-pipe\nbackground\nspaces\nhistory\nctrl+c\nchain\nif\nhelp\nvariables\n"
              "\nBuilt-in commands:\ncd\nexit\nhelp\n" // Falta escribir mas comandos
              "\nTotal: 9.5";
char help_basics[] = "cd:\n"     // LEO
                     "\nexit:\n" // LEO
                     "\n>,<,>>: These features are not considered built-in operators so we send it to the execvp directly, letting the operative system handler them\n"
                     "\n|: If you type command1 | command2, executes command1 and its output is saved to a file, which is later read and sent as input to command2\n"
                     "For more understanding of pipe feature seek how works multi-pipe whit \"help multi-pipe\"\n";
char help_multipipe[] = "With this feature we allow to use multiple pipes in a single command. Example: command1 | command2 | command3 ... and so on.\n"
                        "This is implemented sending the output of each command to a file, which es read later by the next command for input purpose.\n"
                        "We create 2 files in tmp folder and we use a counter variable which it raised by 1 for each command send by the user in the line.\n"
                        "With the counter we save the output following the next behavior: file[count%2] = output and input = file[(count+1)%2], allowing to reuse the same files and save some space\n"
                        "This counter only rise when is used on pipes, because we reuse the method who execute the commands\n";
char help_background[] = ""; // LEO
char help_spaces[] = "For this features we implemented a parse function wich convert the commands like this example: command1 | comand2 into |(command1,command2).\n"
                     "In others words, we convert each operator in a prefix way for convenience.\n"
                     "Of course, in this parse_function we split the string by the space character, ignoring multiple-spaces in a row.\n";
char help_history[] = ""; // LEO
char help_ctrl[] = "";    // LEO
char help_chain[] = "true and false features:\n"
                    "As result of our code, each command put its output in a file, so we treat true and false like echo 0 and echo 1 respectively. For us, 0 is true and 1 is false\n"
                    "; && || features:\n"
                    "For these operators we reuse the way in which we execute the multiple pipes but this time we dont change the counter value (for more understanding execute \"help multi-pipe\" command) and declare that this command do not recive inputs\n"
                    "After executing each operator we check if the previous output of the command (0 or 1) and decide whether to execute the next command depending on its behavior\n";
char help_if[] = "For this features we implemented a parse function that converts the if command1 then command2 else command3 end in if(command1,command2,comman3).\n"
                 "Then we parse each command for later execution.\n"
                 "The actual parse function doesn't allow nested if, but it is relatively easy:\n"
                 "For each clause of the statement of an \"if\", for example: from \"if\" to \"then\", we only have to look for the first appearance of the word \"end\", get its position and compare it with the position of the closing clause. "
                 "If the position of the word \"end\" is less than that of the word \"then\", inside that clause there is an if statement, and we only have to parse that nested if recursively.\n";
char help_variables[] = ""; // LEO

void execute_help(char *function, int count, char *files[])
{
    int o_par = findstr(function, "(");
    int c_par = findstr(function, ")");
    int param_size = c_par - o_par - 1;
    FILE *f = fopen(files[count % 2], "w");
    if (param_size == 0)
        fprintf(f, "%s", help);
    else
    {
        char *param = malloc((param_size + 1) * sizeof(char));
        memset(param, 0, param_size + 1);
        strncat(param, function + o_par + 1, param_size);
        if (strcmp(param, "basics") == 0)
            fprintf(f, "%s", help_basics);
        if (strcmp(param, "multi-pipe") == 0)
            fprintf(f, "%s", help_multipipe);
        if (strcmp(param, "background") == 0)
            fprintf(f, "%s", help_background);
        if (strcmp(param, "spaces") == 0)
            fprintf(f, "%s", help_spaces);
        if (strcmp(param, "history") == 0)
            fprintf(f, "%s", help_history);
        if (strcmp(param, "ctrl+c") == 0)
            fprintf(f, "%s", help_ctrl);
        if (strcmp(param, "chain") == 0)
            fprintf(f, "%s", help_chain);
        if (strcmp(param, "if") == 0)
            fprintf(f, "%s", help_if);
        if (strcmp(param, "variables") == 0)
            fprintf(f, "%s", help_variables);
    }
    fclose(f);
}
int main()
{
    char function[] = "help chain";
    char op[] = "help";
    struct JaggedCharArray split_op = splitstr(op, ' ');
    char *p_function = parse_function(function, split_op);
    execute_help(p_function, 0, files);
}