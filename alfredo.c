#include "cdl-utils.h"
char *files[] = {"cdl-temp0.txt", "cdl-temp1.txt"};
int main()
{
    char command[] = "echo 1 || echo 0";
    char op[] = "; || && | get set if true false";
    char *parse_command = malloc(MAX_COMMAND_LENGTH * sizeof(char));
    parse_command = parse_function(command, splitstr(op, ' '));
    printf("%s\n", parse_command);
    int count = 0;
    main_execute(parse_command, &count, files);
}
