#include "cdl-utils.h"
// TODO: fix error using if commands in linux

// family and the output saved it in a file, and the second command receive the conten
//  from that file as input and save its output in the same file, overwritting its content
int execute(char *command[], bool first);
int main()
{
    char *argv1[] = {"ls", "./", NULL};
    char *argv2[] = {"grep", "test", "input.txt", NULL};
    execute(argv1, true);
    execute(argv2, false);
}
int execute(char *command[], bool first)
{
    int fd_input;
    int fd_output;
    int status;
    pid_t pid = fork();
    if (pid == 0)
    {
        if (first)
        {
            fd_input = open("input.txt", O_RDWR | O_CREAT | O_TRUNC);
            dup2(fd_input, STDOUT_FILENO);
            status = execvp(command[0], command);
            printf("%i\n", status);
            if (status == -1)
            {
                perror("execlp");
                return 1;
            }
        }
        else
        {
            fd_input = open("input.txt", O_RDWR);
            fd_output = open("output.txt", O_RDWR | O_TRUNC);
            dup2(fd_input, STDIN_FILENO);
            dup2(fd_output, STDOUT_FILENO);
            status = execvp(command[0], command);
            printf("%i\n", status);
            if (status == -1)
            {
                perror("execlp");
                return 1;
            }
        }
    }
    else
    {
        waitpid(pid, &status, 0);
        close(fd_input);
        close(fd_output);
        dup2(STDIN_FILENO, STDIN_FILENO);
        dup2(STDOUT_FILENO, STDOUT_FILENO);
    }
    return 0;
}