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

#define MAX_PATH 4096

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
    sig_atomic_t *runningFlag;
};

char *history(struct JaggedCharArray *history, int historyptr, mutex_t *historymutex, bool addnumbers);
int change_dir(char *targetDir);
void *execute_commands(void *args);
int findFreeThread(sig_atomic_t *flagArray);
char *jobs(struct JaggedCharArray *bgcmds, sig_atomic_t *bgcflags, pid *bgpids, mutex_t *bgmutex);
int fg(pid targetpid, struct JaggedCharArray *bgcmds, sig_atomic_t *bgcflags, pid *bgpids, mutex_t *bgmutex);
int set(struct Dictionary *dict, char *var, char *value);
char *get(struct Dictionary dict, char *var);
int unset(struct Dictionary *dict, char *var);

mutex_t fgmutex = PTHREAD_MUTEX_INITIALIZER;
// The foreground thread id
pthread_t foreground;
// The completed flag for the foreground thread
sig_atomic_t *fgcflag;
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
    sig_atomic_t *bgcflags = malloc(MAX_BACKGROUND_PROCESSES * sizeof(sig_atomic_t));
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

    FILE *hf = fopen("history.txt", "r");
    if (hf != NULL)
    {
        char *hist;
        int hlen = readtoend(hf, hist);
        if (hlen != 0)
        {
            struct JaggedCharArray arr = splitstr(hist, '\n');
            free(hist);
            for (i = 0; i < arr.count; i++)
            {
                strcpy(cmdhistory->arr[historyPointer], arr.arr[i]);
                historyPointer = (historyPointer + 1) % HISTORY_LENGTH;
            }
        }
        fclose(hf);
    }

    mutex_t pidmutex = PTHREAD_MUTEX_INITIALIZER;

    pid pidCounter = 0;

    for (i = 0; i < MAX_BACKGROUND_PROCESSES; i++)
        bgcflags[i] = FREE_THREAD;

    signal(SIGINT, sigint_handler);

    char currentDir[MAX_PATH];

    while (true)
    {
        // Wait until the fg thread finishes executing
        while (fgcflag != FREE_THREAD)
            continue;

        if (getcwd(&currentDir, sizeof(currentDir)) == NULL)
        {
            perror(RED "Error acquiring current working directory. Exiting..." COLOR_RESET);
            return 1;
        }

        printf(YELLOW "cdl-bsh" COLOR_RESET " - " CYAN "%s" YELLOW BOLD " $ " BOLD_RESET COLOR_RESET, currentDir);

        fgets(cmd, MAX_COMMAND_LENGTH, stdin);

        int cmdLen = strlen(cmd);

        if (cmdLen == 0)
            continue;

        for (i = 0; i < cmdLen; i++)
            if (cmd[i] == '#')
            {
                cmd[i] = '\0';
                cmdLen = i;
                break;
            }

        // Finding and replacing again tokens
        int againidx = findstr(cmd, "again");
        int againlen = 5;
        int num, numlen, numidx;
        bool error = false;
        while (againidx >= 0)
        {
            numidx = againidx + againlen + 1;
            num = extractint(cmd, numidx, &numlen);

            if (numlen == 0 || num < 1 || num > HISTORY_LENGTH)
            {
                perror(RED BOLD "Invalid AGAIN statement, it's argument must be defined explicitly and be a number between 1 and 10." BOLD_RESET COLOR_RESET "\n");
                error = true;
                break;
            }

            int historyidx = (historyPointer + num - 1) % HISTORY_LENGTH;

            if (cmdhistory->arr[historyidx] == NULL)
            {
                perror(RED BOLD "Invalid AGAIN statement, it's argument must refer to a valid history location, use the history command to see all valid values." BOLD_RESET COLOR_RESET "\n");
                error = true;
                break;
            }

            int historyLen = strlen(cmdhistory->arr[historyidx]);
            if (cmdLen + historyLen - againlen - numlen - 1 > MAX_COMMAND_LENGTH)
            {
                perror(RED BOLD "Invalid AGAIN statement, inserting the command it refers to into the original command would exceed the maximum permited command length." BOLD_RESET COLOR_RESET "\n");
                error = true;
                break;
            }

            replacestr(cmd, cmdhistory->arr[historyidx], againidx, againlen + numlen + 1);

            againidx = findstr(cmd, "again");
        }
        if (error)
            continue;

        if (cmd[0] != ' ')
        {
            lock(&historymutex);
            strcpy(cmdhistory->arr[historyPointer], cmd);
            historyPointer = (historyPointer + 1) % HISTORY_LENGTH;
            unlock(&historymutex);

            FILE *historyfile;
            char *hist = history(cmdhistory, historyPointer, &historymutex, false);

            historyfile = fopen("history.txt", "w");
            fprintf(historyfile, "%s", hist);

            fclose(historyfile);
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
            sig_atomic_t *flag = bgcflags + tindex * sizeof(sig_atomic_t);

            *flag = RUNNING;

            lock(&bgmutex);
            bgargs[tindex] = malloc(sizeof(struct ExecuteArgs));
            strcpy(bgargs[tindex]->cmd, cmd);
            bgargs[tindex]->runningFlag = flag;

            if (pthread_create(&background[tindex], NULL, execute_commands, bgargs[tindex]) != 0)
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

            pidCounter++;
            unlock(&pidmutex);
        }
        else
        {
            lock(&fgmutex);
            *fgcflag = RUNNING;
            fgargs->cmd = cmd;
            fgargs->runningFlag = fgcflag;

            if (pthread_create(&foreground, NULL, execute_commands, fgargs) != 0)
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
    free(bgpids);
    free(background);
    free(bgcflags);
    free(fgcflag);

    return 0;
}

// char *cmd, sig_atomic_t **runningFlag
void *execute_commands(void *args)
{
}

int findFreeThread(sig_atomic_t *flagArray)
{
    int i;
    for (i = 0; i < MAX_BACKGROUND_PROCESSES; i++)
    {
        if (flagArray[i] == FREE_THREAD)
            return i;
    }

    return -1;
}

int change_dir(char *targetDir)
{
    bool valid = is_valid_directory(targetDir);

    if (!valid)
        return 1;

    return -1 * chdir(targetDir); // chdir returns -1 on failure, so we multiply it by -1 to keep things constant as we usually return 1 on errors
}

char *jobs(struct JaggedCharArray *bgcmds, sig_atomic_t *bgcflags, pid *bgpids, mutex_t *bgmutex)
{
    struct JaggedCharArray ret;
    ret.arr = malloc(MAX_BACKGROUND_PROCESSES * sizeof(char *));
    ret.count = MAX_BACKGROUND_PROCESSES;
    int processCount = 0;

    int i;

    lock(bgmutex);
    for (i = 0; i < MAX_BACKGROUND_PROCESSES; i++)
    {
        if (bgcflags[i] == FREE_THREAD)
            continue;

        sprintf(ret.arr[processCount], "[%llu] %s", bgpids[i], bgcmds->arr[i]);
        processCount++;
    }
    unlock(bgmutex);

    char *result = joinarr(ret, '\n', processCount);
    for (i = 0; i < MAX_BACKGROUND_PROCESSES; i++)
        free(ret.arr[i]);
    free(ret.arr);

    return result;
}

// If the user doesn't specify a target pid, it should default to the last pid used (pidCounter - 1)
int fg(pid targetpid, struct JaggedCharArray *bgcmds, sig_atomic_t *bgcflags, pid *bgpids, mutex_t *bgmutex)
{
    lock(bgmutex);
    int index = indexOf(targetpid, bgpids, MAX_BACKGROUND_PROCESSES);

    if (index == -1)
    {
        unlock(bgmutex);
        char *error = malloc(128 * sizeof(char));
        sprintf(error, RED BOLD "Could not find background process with pid " BOLD_RESET YELLOW "%llu" COLOR_RESET, targetpid);
        perror(error);
        free(error);
        return 1;
    }

    lock(&fgmutex);
    strcpy(fgcmd, bgcmds->arr[index]);
    *fgcflag = bgcflags[index];
    fgpid = bgpids[index];
    fgstatus = NONE;

    bgcflags[index] = FREE_THREAD;

    unlock(bgmutex);
    unlock(&fgmutex);

    return 0;
}

char *history(struct JaggedCharArray *history, int historyptr, mutex_t *historymutex, bool addnumbers)
{
    struct JaggedCharArray ret;
    ret.arr = malloc(HISTORY_LENGTH * sizeof(char *));
    ret.count = HISTORY_LENGTH;

    int retIndex = 1;
    int i;

    lock(historymutex);
    for (i = 0; i < HISTORY_LENGTH; i++)
    {
        int index = (historyptr + i) % HISTORY_LENGTH;

        if (history->arr[index] == NULL)
            continue;

        if (addnumbers)
            sprintf(ret.arr[i], "[%d] %s", retIndex, history->arr[index]);
        else
            sprintf(ret.arr[i], "%s", history->arr[index]);

        retIndex++;
    }
    unlock(historymutex);

    char *joined = joinarr(ret, '\n', retIndex - 1);

    for (i = 0; i < HISTORY_LENGTH; i++)
        free(&ret.arr[i]);
    free(ret.arr);

    return joined;
}

int set(struct Dictionary *dict, char *var, char *value)
{
    return dset(dict, var, value);
}

char *listvars(struct Dictionary *dict)
{
    int i;
    int count = dict->count;
    char **lines = malloc(count * sizeof(char *));
    for (i = 0; i < count; i++)
    {
        int keylen = strlen(dict->pairs[i].key);
        int valuelen = strlen(dict->pairs[i].value);
        size_t size = (keylen + valuelen + 2) * sizeof(char);
        lines[i] = malloc(size);
        char *w = lines[i];
        strcpy(w, dict->pairs[i].key);
        w += keylen;
        *w = '=';
        w += 1;
        strcpy(w, dict->pairs[i].value);
        w += valuelen;
        *w = '\0';
    }

    struct JaggedCharArray values = {lines, count};
    char *ret = joinarr(values, '\n', count);
    for (i = 0; i < count; i++)
        free(lines[i]);
    free(lines);
    return ret;
}

char *get(struct Dictionary dict, char *var)
{
    int i = 0;
    return dtryget(dict, var, &i);
}
int unset(struct Dictionary *dict, char *var)
{
    return dremove(dict, var);
}