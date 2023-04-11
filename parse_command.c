#include "cdl-utils.c"

// the parse has the foolowing structure =>  op(destination,source)
char op[] = "| > >> <";
struct JaggedCharArray operators;
int main()
{
    char *command = malloc(MAX_COMMAND_LENGTH * sizeof(char));
    strcat(command, "command1 < file1 | command2");
    operators = splitstr(op, ' ');
    ParseFlow(command);
}
void ParseFlow(char *func)
{
    int functionLen = strlen(func);
    for (int i = 0; i < operators.count; i++)
    {
        }
}