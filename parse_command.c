#include "cdl-utils.c"
// TODO: fix error using if commands in linux

void execute_command(char *func);

int main()
{
    char *p_op = "; if | > >> < ";
    char *p_command = malloc(MAX_COMMAND_LENGTH * sizeof(char));
    memset(p_command, 0, MAX_COMMAND_LENGTH * sizeof(char));
    strcat(p_command, "if   command1 <   file1  then dsa  else sad end");
    printf("%s \n", p_command);
    char *temp = clean_command(p_command);
    printf("%s \n", temp);
    strcpy(p_command, temp);
    free(temp);
    printf("%s \n", p_command);
    struct JaggedCharArray operators = splitstr(p_op, ' ');
    p_command = parse_function(p_command, operators);
    printf("%s \n", p_command);
    free(p_command);
}

bool is_expression(char *func)
{
    for (int i = 0; i < strlen(func); i++)
    {
        if (func[i] == '(')
            return true;
    }
    return false;
}
void execute_command(char *func)
{
    if (!is_expression(func))
        return;
    int len = strlen(func);
    int first_parenthesis = findstr(func, "(");
    char *op = malloc((first_parenthesis + 1) * sizeof(char));
    memset(op, 0, first_parenthesis + 1);
    strncat(op, func, first_parenthesis);
    int count = 0;
    int coma_index = 0;
    for (int i = first_parenthesis + 1; i < len; i++)
    {
        if ((func[i] == ',') && (count == 0))
        {
            coma_index = i;
        }
        if (func[i] == '(')
            count++;
        if (func[i] == ')')
            count--;
    }
    int left_size = coma_index - first_parenthesis;
    char *left = malloc((left_size + 1) * sizeof(char));
    memset(left, 0, left_size + 1);
    strncpy(left, func + first_parenthesis + 1, left_size);
    int right_size = len - 1 - coma_index;
    char *right = malloc((right_size + 1) * sizeof(char));
    memset(right, 0, right_size + 1);
    strncpy(right, func + coma_index + 1, right_size);
    switch (0)
    {
    case 1:
    {
        break;
    }
    default:
        break;
    }
}