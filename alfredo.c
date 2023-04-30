#include "cdl-utils.h"

void main_execute(char *function, int count, char *files[]);
void execute(char *function, int count, char *files[]);

// char *files[] = {"cdl-temp0.txt", "cdl-temp1.txt"};
int main()
{
    char command[] = "set a 5";
    char op[] = "; || && | get set if true false";
    char *parse_command = malloc(MAX_COMMAND_LENGTH * sizeof(char));
    parse_command = parse_function(command, splitstr(op, ' '));
    printf("%s\n", parse_command);
}
