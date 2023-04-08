#include "cdl-utils.h"

#define MAX_COMMAND_LENGTH 8192

void *change_dir(void *arg);
int execute_commands(char *cmd, char *currentDir);

int main()
{
    // To store the last command entered by the user.
    char *cmd = malloc(MAX_COMMAND_LENGTH * sizeof(char));

    char currentDir[PATH_MAX];
    if (getcwd(currentDir, sizeof(currentDir)) == NULL)
    {
        perror(RED "Error acquiring current working directory. Exiting..." COLOR_RESET);
        return 1;
    }

    while (true)
    {
        printf(YELLOW "cdl-bsh" COLOR_RESET " - " CYAN "%s" YELLOW BOLD " $ " BOLD_RESET COLOR_RESET, currentDir);

        fgets(cmd, MAX_COMMAND_LENGTH, stdin);

        if (execute_commands(cmd, &currentDir) != 0)
            break;
    }

    free(cmd);

    return 0;
}

int execute_commands(char *cmd, char *currentDir)
{
}

// arg = [targetDir(char *), dirVariable(char *)]
void *change_dir(void *arg)
{
    size_t pchar_size = sizeof(char *);

    char *target = malloc(pchar_size);
    char *var = malloc(pchar_size);
    memcpy(target, arg, pchar_size);
    memcpy(var, arg + pchar_size, pchar_size);

    if (!is_valid_directory(target))
        return 1;

    *var = *target;
    return 0;
}
