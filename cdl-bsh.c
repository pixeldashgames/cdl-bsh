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
#include <signal.h>

#define MAX_COMMAND_LENGTH 8192
#define MAX_BACKGROUND_PROCESSES 1024

#define FREE_THREAD 0
#define RUNNING 1

#define NONE 0
#define CANCELLED 1

#define HISTORY_LENGTH 10

#define mutex_t pthread_mutex_t
#define lock(mutex) pthread_mutex_lock(mutex)
#define unlock(mutex) pthread_mutex_unlock(mutex)

typedef unsigned long long pid;

struct ExecuteArgs
{
    char *cmd;
    char *currentDir;
    volatile sig_atomic_t *runningFlag;
};

int change_dir(char *targetDir, char *dirVariable, mutex_t *cwdmutex);
void *execute_commands(void *args);
int findFreeThread(volatile sig_atomic_t *flagArray);
char *jobs(struct JaggedCharArray *bgcmds, volatile sig_atomic_t *bgcflags, pid *bgpids, mutex_t *bgmutex);
int fg(pid targetpid, struct JaggedCharArray *bgcmds, volatile sig_atomic_t *bgcflags, pid *bgpids, mutex_t *bgmutex);

mutex_t fgmutex = PTHREAD_MUTEX_INITIALIZER;
// The foreground thread id
pthread_t foreground;
// The completed flag for the foreground thread
volatile sig_atomic_t *fgcflag;
// The processID of the foreground thread
pid fgpid;
// The args of the foregroundThread
struct ExecuteArgs *fgargs;
// The command being processed by the fg thread
char *fgcmd;
int fgstatus = NONE;

void sigint_handler(int sig)
{
    if (*fgcflag == FREE_THREAD)
        return;

    lock(&fgmutex);

    if (fgstatus == NONE)
    {
        pthread_cancel(foreground);
        fgstatus = CANCELLED;
    }
    else
    {
        pthread_kill(foreground, SIGTERM);
    }

    unlock(&fgmutex);
}

int main()
{
    int i;

    // To store the last command entered by the user.
    char *cmd = malloc(MAX_COMMAND_LENGTH * sizeof(char));

    mutex_t bgmutex = PTHREAD_MUTEX_INITIALIZER;

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

    fgcflag = malloc(sizeof(sig_atomic_t));
    fgargs = malloc(sizeof(struct ExecuteArgs));
    fgcmd = malloc(MAX_COMMAND_LENGTH * sizeof(char));

    mutex_t historymutex = PTHREAD_MUTEX_INITIALIZER;

    struct JaggedCharArray *cmdhistory = malloc(sizeof(struct JaggedCharArray));
    cmdhistory->count = HISTORY_LENGTH;
    cmdhistory->arr = malloc(HISTORY_LENGTH * sizeof(char *));
    int historyPointer = 0;

    mutex_t pidmutex = PTHREAD_MUTEX_INITIALIZER;

    pid pidCounter = 0;

    for (i = 0; i < MAX_BACKGROUND_PROCESSES; i++)
        bgcflags[i] = FREE_THREAD;

    mutex_t cwdmutex = PTHREAD_MUTEX_INITIALIZER;

    char currentDir[PATH_MAX];
    if (getcwd(currentDir, sizeof(currentDir)) == NULL)
    {
        perror(RED "Error acquiring current working directory. Exiting..." COLOR_RESET);
        return 1;
    }

    signal(SIGINT, sigint_handler);

    while (true)
    {
        // Wait until the fg thread finishes executing
        while (fgcflag != FREE_THREAD)
            continue;

        lock(&cwdmutex);
        printf(YELLOW "cdl-bsh" COLOR_RESET " - " CYAN "%s" YELLOW BOLD " $ " BOLD_RESET COLOR_RESET, currentDir);
        unlock(&cwdmutex);

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

            lock(&bgmutex);
            bgargs[tindex] = malloc(sizeof(struct ExecuteArgs));
            strcpy(bgargs[tindex]->cmd, cmd);
            lock(&cwdmutex);
            strcpy(bgargs[tindex]->currentDir, currentDir);
            unlock(&cwdmutex);
            bgargs[tindex]->runningFlag = flag;

            if (pthread_create(background[tindex], NULL, execute_commands, bgargs[tindex]) != 0)
            {
                perror(RED BOLD "Error creating command thread" BOLD_RESET COLOR_RESET "\n");
                *flag = FREE_THREAD;
                unlock(&bgmutex);
                continue;
            }

            // Thread created succesfully
            strcpy(bgcmds->arr[tindex], cmd);
            lock(&pidmutex);
            bgpids[tindex] = pidCounter;
            unlock(&bgmutex);
            lock(&historymutex);
            strcpy(cmdhistory->arr[historyPointer], cmd);
            historyPointer = (historyPointer + 1) % HISTORY_LENGTH;
            unlock(&historymutex);
            pidCounter++;
            unlock(&pidmutex);
        }
        else
        {
            lock(&fgmutex);
            *fgcflag = RUNNING;
            fgargs->cmd = cmd;
            lock(&cwdmutex);
            strcpy(fgargs->currentDir, currentDir);
            unlock(&cwdmutex);
            fgargs->runningFlag = fgcflag;

            if (pthread_create(foreground, NULL, execute_commands, fgargs) != 0)
            {
                unlock(&fgmutex);
                perror(RED BOLD "Error creating command thread" BOLD_RESET COLOR_RESET "\n");
                *fgcflag = FREE_THREAD;
                continue;
            }

            fgstatus = NONE;
            strcpy(fgcmd, cmd);
            lock(&pidmutex);
            fgpid = pidCounter;
            lock(&historymutex);
            strcpy(cmdhistory->arr[historyPointer], cmd);
            historyPointer = (historyPointer + 1) % HISTORY_LENGTH;
            unlock(&historymutex);
            pidCounter++;
            unlock(&pidmutex);
            unlock(&fgmutex);
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

int change_dir(char *targetDir, char *dirVariable, mutex_t *cwdmutex)
{
    lock(&cwdmutex);
    bool valid = is_valid_directory(targetDir);

    if (!valid)
    {
        unlock(&cwdmutex);
        return 1;
    }

    *dirVariable = *targetDir;
    unlock(&cwdmutex);
    return 0;
}

char *jobs(struct JaggedCharArray *bgcmds, volatile sig_atomic_t *bgcflags, pid *bgpids, mutex_t *bgmutex)
{
    struct JaggedCharArray ret;
    ret.arr = malloc(MAX_BACKGROUND_PROCESSES * sizeof(char *));
    ret.count = MAX_BACKGROUND_PROCESSES;
    int processCount = 0;

    int i;

    lock(&bgmutex);
    for (i = 0; i < MAX_BACKGROUND_PROCESSES; i++)
    {
        if (bgcflags[i] == FREE_THREAD)
            continue;

        sprintf(&ret.arr[processCount], "[%d] %s", bgpids[i], bgcmds->arr[i]);
        processCount++;
    }
    unlock(&bgmutex);

    char *result = joinarr(ret, '\n', processCount);
    for (i = 0; i < MAX_BACKGROUND_PROCESSES; i++)
        free(&ret.arr[i]);
    free(ret.arr);

    return result;
}

// If the user doesn't specify a target pid, it should default to the last pid used (pidCounter - 1)
int fg(pid targetpid, struct JaggedCharArray *bgcmds, volatile sig_atomic_t *bgcflags, pid *bgpids, mutex_t *bgmutex)
{
    lock(&bgmutex);
    int index = indexOf(targetpid, bgpids, MAX_BACKGROUND_PROCESSES);

    if (index == -1)
    {
        unlock(&bgmutex);
        char error[MAX_COMMAND_LENGTH];
        sprintf(&error, RED BOLD "Could not find background process with pid " BOLD_RESET YELLOW "%d" COLOR_RESET, targetpid);
        perror(error);
        return 1;
    }

    lock(&fgmutex);
    strcpy(fgcmd, bgcmds->arr[index]);
    *fgcflag = bgcflags[index];
    fgpid = bgpids[index];
    fgstatus = NONE;

    bgcflags[index] = FREE_THREAD;

    unlock(&bgmutex);
    unlock(&fgmutex);

    return 0;
}

char *history(struct JaggedCharArray *history, int historyptr, mutex_t *historymutex)
{
    struct JaggedCharArray ret;
    ret.arr = malloc(HISTORY_LENGTH * sizeof(char *));
    ret.count = HISTORY_LENGTH;

    int retIndex = 1;
    int i;

    lock(&historymutex);
    for (i = 0; i < HISTORY_LENGTH; i++)
    {
        int index = (historyptr + i) % HISTORY_LENGTH;

        if (history->arr[index] == NULL)
            continue;

        sprintf(&ret.arr[i], "[%d] %s", retIndex, history->arr[index]);
        retIndex++;
    }
    unlock(&historymutex);

    char *joined = joinarr(ret, '\n', retIndex - 1);

    for (i = 0; i < HISTORY_LENGTH; i++)
        free(&ret.arr[i]);
    free(ret.arr);

    return joined;
}
