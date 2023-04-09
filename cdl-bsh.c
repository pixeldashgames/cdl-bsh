#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <pthread.h>
#include "cdl-text-utils.h"
#include "cdl-utils.h"

#define MAX_COMMAND_LENGTH 8192
#define MAX_BACKGROUND_PROCESSES 1024

#define FREE_THREAD 0
#define RUNNING 1

typedef unsigned long long pid;

struct ExecuteArgs
{
    char *cmd;
    char *currentDir;
    volatile sig_atomic_t *runingFlag;
};

int change_dir(char *targetDir, char *dirVariable);
void *execute_commands(void *args);
int findFreeThread(volatile sig_atomic_t *flagArray);
char *jobs(struct JaggedCharArray *bgcmds, volatile sig_atomic_t *bgcflags, pid *bgpids);

int main()
{
    int i;

    // To store the last command entered by the user.
    char *cmd = malloc(MAX_COMMAND_LENGTH * sizeof(char));

    // Storing background threads
    pthread_t *background = malloc(MAX_BACKGROUND_PROCESSES * sizeof(pthread_t));
    // This will store whether each background thread has finished its execution
    volatile sig_atomic_t *bgcflags = malloc(MAX_BACKGROUND_PROCESSES * sizeof(sig_atomic_t));
    // This will store the pid of each background thread
    pid *bgpids = malloc(MAX_BACKGROUND_PROCESSES * sizeof(pid));
    // This stores the execution args of every running thread.
    struct ExecuteArgs **bgargs = malloc(MAX_BACKGROUND_PROCESSES * sizeof(struct ExecuteArgs *));
    // This stores the command being processed by every background thread
    struct JaggedCharArray *bgcmds = malloc(sizeof(struct JaggedCharArray));
    bgcmds->count = MAX_BACKGROUND_PROCESSES;
    bgcmds->arr = malloc(MAX_BACKGROUND_PROCESSES * sizeof(char *));

    // The foreground thread id
    pthread_t foreground;
    // The completed flag for the foreground thread
    volatile sig_atomic_t *fgcflag = malloc(sizeof(sig_atomic_t));
    // The processID of the foreground thread
    pid fgpid;
    // The args of the foregroundThread
    struct ExecuteArgs *fgargs = mallloc(sizeof(struct ExecuteArgs));
    // The command being processed by the fg thread
    char *fgcmd = malloc(MAX_COMMAND_LENGTH * sizeof(char));

    pid pidCounter = 0;

    for (i = 0; i < MAX_BACKGROUND_PROCESSES; i++)
        bgcflags[i] = FREE_THREAD;

    char currentDir[PATH_MAX];
    if (getcwd(currentDir, sizeof(currentDir)) == NULL)
    {
        perror(RED "Error acquiring current working directory. Exiting..." COLOR_RESET);
        return 1;
    }

    while (true)
    {
        // Wait until the fg thread finishes executing
        while (fgcflag != FREE_THREAD)
            continue;

        printf(YELLOW "cdl-bsh" COLOR_RESET " - " CYAN "%s" YELLOW BOLD " $ " BOLD_RESET COLOR_RESET, currentDir);

        fgets(cmd, MAX_COMMAND_LENGTH, stdin);

        int cmdLen = strlen(cmd);

        for (i = 0; i < cmdLen; i++)
            if (cmd[i] == '#')
            {
                cmd[i] = '\0';
                cmdLen = i;
                break;
            }

        if (cmd[cmdLen - 1] == '&')
        {
            cmd[cmdLen - 1] = '\0';
            // Find a free thread or wait until one is fred
            int tindex;
            do
            {
                tindex = findFreeThread(bgcflags);
            } while (tindex < 0);

            // Ptr to the running flag for the new thread
            volatile sig_atomic_t *flag = bgcflags + tindex * sizeof(sig_atomic_t);

            *flag = RUNNING;

            bgargs[tindex] = malloc(sizeof(struct ExecuteArgs));
            bgargs[tindex]->cmd = cmd;
            bgargs[tindex]->currentDir = &currentDir;
            bgargs[tindex]->runingFlag = flag;

            if (pthread_create(background[tindex], NULL, execute_commands, bgargs[tindex]) != 0)
            {
                perror(RED BOLD "Error creating command thread" BOLD_RESET COLOR_RESET "\n");
                *flag = FREE_THREAD;
                continue;
            }

            // Thread created succesfully
            bgcmds->arr[tindex] = *cmd;
            bgpids[tindex] = pidCounter;
            pidCounter++;
        }
        else
        {
            *fgcflag = RUNNING;
            fgargs->cmd = cmd;
            fgargs->currentDir = &currentDir;
            fgargs->runingFlag = fgcflag;

            if (pthread_create(foreground, NULL, execute_commands, fgargs) != 0)
            {
                perror(RED BOLD "Error creating command thread" BOLD_RESET COLOR_RESET "\n");
                *fgcflag = FREE_THREAD;
                continue;
            }

            *fgcmd = *cmd;
            fgpid = pidCounter;
            pidCounter++;
        }
    }

    for (i = 0; i < MAX_BACKGROUND_PROCESSES; i++)
    {
        free(bgargs[i]);
        free(bgcmds->arr[i]);
    }
    free(bgcmds);
    free(fgcmd);
    free(fgargs);
    free(bgargs);
    free(cmd);
    cree(bgpids);
    free(background);
    free(bgcflags);
    free(fgcflag);

    return 0;
}

// char *cmd, char *currentDir, sig_atomic_t **runningFlag
void *execute_commands(void *args)
{
}

int findFreeThread(volatile sig_atomic_t *flagArray)
{
    int i;
    for (i = 0; i < MAX_BACKGROUND_PROCESSES; i++)
    {
        if (flagArray[i] == FREE_THREAD)
            return i;
    }

    return -1;
}

int change_dir(char *targetDir, char *dirVariable)
{
    if (!is_valid_directory(targetDir))
        return 1;

    *dirVariable = *targetDir;
    return 0;
}

char *jobs(struct JaggedCharArray *bgcmds, volatile sig_atomic_t *bgcflags, pid *bgpids)
{
    struct JaggedCharArray ret;
    ret.arr = malloc(MAX_BACKGROUND_PROCESSES * sizeof(char *));
    ret.count = MAX_BACKGROUND_PROCESSES;
    int processCount = 0;

    int i;
    for (i = 0; i < MAX_BACKGROUND_PROCESSES; i++)
    {
        if (bgcflags[i] == FREE_THREAD)
            continue;

        sprintf(&ret.arr[processCount], "[%d] %s", bgpids[i], bgcmds->arr[i]);
        processCount++;
    }

    

    return joinarr(ret, '\n', processCount);
}

// If the user doesn't specify a target pid, it should default to the last pid used (pidCounter - 1)
int fg(pid targetpid, struct JaggedCharArray *bgcmds, volatile sig_atomic_t *bgcflags, pid *bgpids, char *fgcmd, volatile sig_atomic_t *fgcflag, pid *fgpid)
{
    int index = indexOf(targetpid, bgpids, MAX_BACKGROUND_PROCESSES);

    if(index == -1)
    {
        char error[MAX_COMMAND_LENGTH];
        sprintf(&error, RED BOLD "Could not find background process with pid " BOLD_RESET YELLOW "%d" COLOR_RESET, targetpid);
        perror(error);
        return 1;
    }

    strcpy(fgcmd, bgcmds->arr[index]);
    *fgcflag = bgcflags[index];
    *fgpid = bgpids[index];

    bgcflags[index] = FREE_THREAD;

    return 0;
}


