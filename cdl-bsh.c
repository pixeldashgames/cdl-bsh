#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <limits.h>
#include <dirent.h>
#include <pthread.h>
#include "cdl-text-utils.h"
#include "cdl-utils.h"
#include <signal.h>

#define MAX_BACKGROUND_PROCESSES 1024
#define MAX_COMMAND_LENGTH 8192

#define MAX_PATH 4096

#define VARS_CAPACITY 1024

#define FREE_THREAD 0
#define RUNNING 1

#define NONE 0
#define CANCELLED 1

#define DATA_FOLDER ".cdl-bsh/"
#define HISTORY_FILE ".cdl-bsh/history"

#define HISTORY_LENGTH 10

#define mutex_t pthread_mutex_t
#define lock(mutex) pthread_mutex_lock(mutex)
#define unlock(mutex) pthread_mutex_unlock(mutex)

typedef unsigned long long pid;

struct ExecuteArgs
{
    sig_atomic_t *runningFlag;
    char *cmd;
    pid pidCounter;
    struct JaggedCharArray *bgcmds;
    sig_atomic_t *bgcflags;
    pid *bgpids;
    mutex_t *bgmutex;
    struct JaggedCharArray *history;
    int historyptr;
    mutex_t *historymutex;
    mutex_t *varsmutex;
    struct Dictionary *varsDict;
};

int execute_pipe(char *command[], bool first, char *files[], int count);
void main_execute(char *function, int *count, char *files[], bool *First, struct ExecuteArgs executeArgs);
void execute_nonboolean(char *function, int *count, char *files[], char *op, bool *First, struct ExecuteArgs executeArgs);
void execute_boolean(char *function, int *count, char *files[], char *op, bool *First, struct ExecuteArgs executeArgs);
void cleanup_function(void *arg);
char *read_file(char *files[], int count, bool *First);

char *history(struct JaggedCharArray *history, int historyptr, mutex_t *historymutex, bool addnumbers);
int change_dir(char *targetDir);
void *execute_commands(void *args);
int findFreeThread(sig_atomic_t *flagArray);
char *jobs(struct JaggedCharArray *bgcmds, sig_atomic_t *bgcflags, pid *bgpids, mutex_t *bgmutex);
int fg(pid targetpid, struct JaggedCharArray *bgcmds, sig_atomic_t *bgcflags, pid *bgpids, mutex_t *bgmutex);
int set(struct Dictionary *dict, mutex_t *varsmutex, char *var, char *value);
char *get(struct Dictionary dict, mutex_t *varsmutex, char *var);
int unset(struct Dictionary *dict, mutex_t *varsmutex, char *var);

char *if_file[] = {"cdl-if-temp.txt"};

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
    char cmd[MAX_COMMAND_LENGTH];

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
    *fgcflag = FREE_THREAD;
    fgargs = malloc(sizeof(struct ExecuteArgs));
    fgcmd = malloc(MAX_COMMAND_LENGTH * sizeof(char));

    mutex_t historymutex = PTHREAD_MUTEX_INITIALIZER;

    struct JaggedCharArray *cmdhistory = malloc(sizeof(struct JaggedCharArray));
    cmdhistory->count = HISTORY_LENGTH;
    cmdhistory->arr = malloc(HISTORY_LENGTH * sizeof(char *));
    for (i = 0; i < HISTORY_LENGTH; i++)
    {
        cmdhistory->arr[i] = malloc(MAX_COMMAND_LENGTH * sizeof(char));
        cmdhistory->arr[i][0] = '\0';
    }
    int historyPointer = 0;

    char *historysp = HISTORY_FILE;
    char *history_file = get_home_subpath(historysp);

    FILE *hf = fopen(history_file, "r");
    if (hf != NULL)
    {
        char *f[] = {history_file};
        bool b = true;
        char *hist = read_file(f, 0, &b);
        int hlen = strlen(hist);

        if (hlen != 0)
        {
            struct JaggedCharArray arr = splitstr(hist, '\n');
            free(hist);
            for (i = 0; i < arr.count; i++)
            {
                cmdhistory->arr[historyPointer] = malloc(MAX_COMMAND_LENGTH * sizeof(char));
                strcpy(cmdhistory->arr[historyPointer], arr.arr[i]);
                historyPointer = (historyPointer + 1) % HISTORY_LENGTH;
            }
        }
        fclose(hf);
    }
    else
    {
        char *datasp = DATA_FOLDER;
        char *datafldr = get_home_subpath(datasp);

        if (!directory_exists(datafldr))
        {
            mkdir(datafldr, 0755);
        }

        free(datafldr);
    }

    mutex_t varsmutex = PTHREAD_MUTEX_INITIALIZER;
    struct Dictionary *varsDict = malloc(sizeof(struct Dictionary));
    varsDict->pairs = malloc(VARS_CAPACITY * sizeof(struct KeyValuePair));
    varsDict->count = VARS_CAPACITY;

    mutex_t pidmutex = PTHREAD_MUTEX_INITIALIZER;

    pid pidCounter = 0;

    for (i = 0; i < MAX_BACKGROUND_PROCESSES; i++)
        bgcflags[i] = FREE_THREAD;

    signal(SIGINT, sigint_handler);

    while (true)
    {
        // Wait until the fg thread finishes executing
        while (*fgcflag != FREE_THREAD)
            continue;

        char *currentDir = getcwd(NULL, 0);
        if (currentDir == NULL)
        {
            perror(RED "Error acquiring current working directory. Exiting..." COLOR_RESET);
            return 1;
        }

        printf(YELLOW "cdl-bsh" COLOR_RESET " - " CYAN "%s" YELLOW BOLD " $ " BOLD_RESET COLOR_RESET, currentDir);

        fgets(cmd, sizeof(cmd), stdin);

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

        if (cmd[cmdLen - 1] == '\n')
        {
            cmd[cmdLen - 1] = '\0';
            cmdLen -= 1;
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

        printf("a");
        fflush(stdout);

        if (cmd[0] != ' ')
        {
            lock(&historymutex);

            if (cmdhistory->arr[historyPointer] == NULL)
            {
                cmdhistory->arr[historyPointer] = malloc(MAX_COMMAND_LENGTH * sizeof(char));
            }

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
            bgargs[tindex]->bgcflags = bgcflags;
            bgargs[tindex]->bgcmds = bgcmds;
            bgargs[tindex]->bgmutex = &bgmutex;
            bgargs[tindex]->bgpids = bgpids;
            bgargs[tindex]->history = cmdhistory;
            bgargs[tindex]->historymutex = &historymutex;
            bgargs[tindex]->historyptr = historyPointer;
            bgargs[tindex]->pidCounter = pidCounter;
            bgargs[tindex]->varsDict = varsDict;
            bgargs[tindex]->varsmutex = &varsmutex;

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
            fgargs->bgcflags = bgcflags;
            fgargs->bgcmds = bgcmds;
            fgargs->bgmutex = &bgmutex;
            fgargs->bgpids = bgpids;
            fgargs->history = cmdhistory;
            fgargs->historymutex = &historymutex;
            fgargs->historyptr = historyPointer;
            fgargs->pidCounter = pidCounter;
            fgargs->varsDict = varsDict;
            fgargs->varsmutex = &varsmutex;

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
    free(history_file);
    free(bgcmds);
    free(fgcmd);
    free(fgargs);
    free(bgargs);
    free(cmd);
    free(bgpids);
    free(background);
    free(bgcflags);
    free(fgcflag);
    free(varsDict->pairs);
    free(varsDict);

    return 0;
}

// ExecuteArgs
void *execute_commands(void *args)
{
    struct ExecuteArgs *arg = (struct ExecuteArgs *)args;

    pthread_cleanup_push(cleanup_function, arg->runningFlag);

    char op[] = "; || && | if true false help cd jobs fg history set get unset exit";

    char *pcmd = parse_function(arg->cmd, splitstr(op, ' '));

    struct timeval tv;
    gettimeofday(&tv, NULL);

    char time_string[30];
    time_t current_time = tv.tv_sec;
    struct tm *local_time = localtime(&current_time);
    strftime(time_string, sizeof(time_string), "%Y-%m-%dT%H-%M-%S", local_time);
    sprintf(time_string + strlen(time_string), "-%03d", (int)tv.tv_usec / 1000);

    char *file1path = malloc(50 * sizeof(char));
    char *file2path = malloc(50 * sizeof(char));
    char *file3path = malloc(50 * sizeof(char));

    sprintf(file1path, "/tmp/cdl-bsh-%s.one.tmp", time_string);
    sprintf(file2path, "/tmp/cdl-bsh-%s.two.tmp", time_string);
    sprintf(file3path, "/tmp/cdl-bsh-%s.if.tmp", time_string);

    char **files = malloc(3 * sizeof(char *));
    files[0] = file1path;
    files[1] = file2path;
    files[2] = file3path;

    int count = 0;
    bool First = true;
    printf("%s\n", pcmd);
    main_execute(pcmd, &count, files, &First, *arg);

    pthread_cleanup_pop(1);
}

// sig_atomic_t *runningFlag
void cleanup_function(void *arg)
{
    sig_atomic_t *rflag = (sig_atomic_t *)arg;

    *rflag = FREE_THREAD;
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
    struct JaggedCharArray ret = {
        malloc(HISTORY_LENGTH * sizeof(char *)), HISTORY_LENGTH};

    for (int i = 0; i < HISTORY_LENGTH; i++)
    {
        ret.arr[i] = malloc(MAX_COMMAND_LENGTH * sizeof(char));
    }

    int retIndex = 1;
    int i;

    lock(historymutex);
    for (i = 0; i < HISTORY_LENGTH; i++)
    {
        int index = (historyptr + i) % HISTORY_LENGTH;

        if (history->arr[index][0] == '\0')
            continue;

        if (addnumbers)
            sprintf(ret.arr[retIndex - 1], "[%d] %s", retIndex, history->arr[index]);
        else
            sprintf(ret.arr[retIndex - 1], "%s", history->arr[index]);

        retIndex++;
    }
    unlock(historymutex);

    char *joined = joinarr(ret, '\n', retIndex - 1);

    for (i = 0; i < HISTORY_LENGTH; i++)
        free(ret.arr[i]);
    free(ret.arr);

    return joined;
}

int set(struct Dictionary *dict, mutex_t *varsmutex, char *var, char *value)
{
    lock(varsmutex);
    int ret = dset(dict, var, value);
    unlock(varsmutex);

    return ret;
}

char *listvars(struct Dictionary *dict, mutex_t *varsmutex)
{
    int i;
    int count = dict->count;
    char **lines = malloc(count * sizeof(char *));

    lock(varsmutex);
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
    unlock(varsmutex);
    struct JaggedCharArray values = {lines, count};
    char *ret = joinarr(values, '\n', count);
    for (i = 0; i < count; i++)
        free(lines[i]);
    free(lines);
    return ret;
}

char *get(struct Dictionary dict, mutex_t *varsmutex, char *var)
{
    lock(varsmutex);
    int i = 0;
    char *ret = dtryget(dict, var, &i);
    unlock(varsmutex);

    return ret;
}

int unset(struct Dictionary *dict, mutex_t *varsmutex, char *var)
{
    lock(varsmutex);
    int ret = dremove(dict, var);
    unlock(varsmutex);

    return ret;
}

char *clean_command(char *func)
{
    struct JaggedCharArray command_clean = splitstr(func, ' ');
    for (int i = 0; i < command_clean.count; i++)
    {
        printf("%s \n", command_clean.arr[i]);
    }

    return joinarr(command_clean, ' ', command_clean.count);
}

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
                int result_len = strlen(func) + 3;
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

int execute_pipe(char *command[], bool first, char *files[], int count)
{
    printf("first: %s, count: %i\n", (first) ? "true" : "false", count);
    int fd_input = -1;
    int fd_output = -1;
    int status;
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
    pid_t pid = fork();
    if (pid == 0)
    {
        if (first)
        {
            fd_output = open(files[count % 2], O_WRONLY | O_CREAT | O_TRUNC, mode);
            if (fd_output == -1)
            {
                perror("open");
                exit(EXIT_FAILURE);
            }
            close(STDOUT_FILENO);
            dup2(fd_output, STDOUT_FILENO);
            close(fd_output);
            status = execvp(command[0], command);
            perror("execvp");
            exit(EXIT_FAILURE);
        }
        else
        {
            fd_input = open(files[(count + 1) % 2], O_RDONLY, mode);
            fd_output = open(files[count % 2], O_WRONLY | O_CREAT | O_TRUNC, mode);
            close(STDOUT_FILENO);
            dup2(fd_input, STDIN_FILENO);
            dup2(fd_output, STDOUT_FILENO);
            close(fd_input);
            close(fd_output);
            status = execvp(command[0], command);
            perror("execvp");
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        waitpid(pid, &status, 0);
        close(fd_input);
        close(fd_output);
        // remove(files[(count + 1) % 2]);
    }
    return 0;
}

void copy_string_array(char **src, char **dest, int size)
{
    for (int i = 0; i < size; i++)
    {
        dest[i] = malloc((strlen(src[i]) + 1) * sizeof(char));
        memset(dest[i], 0, strlen(src[i] + 1));
        strcpy(dest[i], src[i]);
    }
}

bool is_command(char *function)
{
    int len = strlen(function);
    for (int i = 0; i < len; i++)
    {
        if (function[i] == '(')
            return false;
    }
    return true;
}

char *read_file(char *files[], int count, bool *First)
{
    FILE *fp;
    long lSize;
    char *buffer;
    fp = fopen((*First) ? files[count % 2] : files[(count + 1) % 2], "rb");
    if (!fp)
        perror("file not found"), exit(1);

    fseek(fp, 0L, SEEK_END);
    lSize = ftell(fp);
    rewind(fp);

    buffer = calloc(1, lSize + 1);
    if (!buffer)
        fclose(fp), fputs("Memory alloc fails", stderr), exit(1);

    if (1 != fread(buffer, lSize, 1, fp))
        fclose(fp), free(buffer), fputs("Entire read fails", stderr), exit(1);

    fclose(fp);
    return buffer;
}

char *getcmdinput(bool First, bool noargs, char **files, int count, struct JaggedCharArray argsarr)
{
    if (!First && noargs)
    {
        FILE *infile = fopen(files[(count + 1) % 2], "r");

        if (infile != NULL)
        {
            char *f[] = {files[(count + 1) % 2]};
            char *out = read_file(f, 0, &First);
            if (strlen(out) == 0)
            {
                fclose(infile);

                return NULL;
            }

            fclose(infile);
            int outlen = strlen(out);
            if (out[outlen - 1] == '\n')
                out[outlen - 1] = '\0';
            return out;
        }
        else
        {
            return NULL;
        }
    }
    else if (First)
    {
        if (noargs)
        {
            return NULL;
        }
        else
        {
            char *out = joinarr(argsarr, ' ', argsarr.count);
            int outlen = strlen(out);
            if (out[outlen - 1] == '\n')
                out[outlen - 1] = '\0';
            return out;
        }
    }
}

// Para saber el archivo a guardar/leer el output de los metodos es files[count%2]
// Para abrir el archivo:
// mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
// fd_output = open(files[count % 2], O_WRONLY | O_CREAT | O_TRUNC, mode);
//
// Despues de terminar de trabajar el archivo: close(fd_output)

// Cada comportamiento de los operadores a implementar se programa dentro de un if
// Falta crear el comportamiento del comando help

// Para ejecutar el shell desde cdl-bsh primero se llama a parse_function(command, splitstr(op, ' '));
// Luego a main_execute(parsed_function,0,files) siendo files la direcion de los archivos de salida q se van a generar en tmp/
// Falta cambiar la direcion de *if_file[] inicializado al inicio de este .c y ubicarlo en tmp/ tambien

// Para debuggear sin conectarse al shell ejecutar: gcc alfredo.c cdl-utils.c -o test
// Testeando en el archivo alfredo.c
void main_execute(char *function, int *count, char *files[], bool *First, struct ExecuteArgs executeArgs)
{
    if (is_command(function))
    {
        struct JaggedCharArray split_func = splitstr(function, ' ');
        char **new_func = malloc((split_func.count + 1) * sizeof(char *));
        copy_string_array(split_func.arr, new_func, split_func.count);
        new_func[split_func.count] = malloc(sizeof(NULL));
        new_func[split_func.count] = NULL;
        int f = execute_pipe(new_func, *First, files, *count);
        *First = false;
        free(new_func);
        return;
    }

    int parenthesis_init = findstr(function, "(");
    char *op = malloc((parenthesis_init + 1) * sizeof(char));
    memset(op, 0, parenthesis_init + 1);
    strncpy(op, function, parenthesis_init);
    int op_len = strlen(op);

    char *args = function + parenthesis_init + 1;
    int alen = strlen(args);

    bool noargs = false;

    struct JaggedCharArray argsarr;

    if (alen <= 1)
    {
        noargs = true;
    }
    else
    {
        args = malloc(alen * sizeof(char));

        strncpy(args, function + parenthesis_init + 1, alen - 1);
        args[alen] = '\0';

        argsarr = splitstr(args, ',');
    }

    if (op_len == 1 && op[0] == '|')
    {
        execute_nonboolean(function, &(*count), files, op, &(*First), executeArgs);
        return;
    }
    if (op_len == 1 && op[0] == ';')
    {
        execute_nonboolean(function, &(*count), files, op, &(*First), executeArgs);
        *First = true;
        return;
    }
    if (strcmp(op, "cd") == 0)
    {
        char *arg = getcmdinput(First, noargs, files, *count, argsarr);

        if (arg == NULL)
        {
            perror(RED "The cd command requires 1 argument" COLOR_RESET);
            return;
        }

        int result = change_dir(arg);

        FILE *outfile = fopen(files[*count % 2], "w");

        fprintf(outfile, "%d", result);
        fclose(outfile);
        return;
    }
    if (strcmp(op, "fg") == 0)
    {
        int targetpid;
        char *arg = getcmdinput(First, noargs, files, *count, argsarr);

        if (arg != NULL)
        {
            targetpid = atoi(arg);
        }
        else
        {
            targetpid = executeArgs.pidCounter - 1;
        }

        int result = fg(targetpid, executeArgs.bgcmds, executeArgs.bgcflags,
                        executeArgs.bgpids, executeArgs.bgmutex);

        FILE *outfile = fopen(files[*count % 2], "w");

        fprintf(outfile, "%d", result);
        fclose(outfile);
        return;
    }
    if (strcmp(op, "exit") == 0)
    {
        printf("\nBye bye!\n");
        exit(0);
    }
    if (strcmp(op, "jobs") == 0)
    {
        char *result = jobs(executeArgs.bgcmds, executeArgs.bgcflags,
                            executeArgs.bgpids, executeArgs.bgmutex);

        FILE *outfile = fopen(files[*count % 2], "w");

        fprintf(outfile, "%s", result);
        fclose(outfile);
        return;
    }
    if (strcmp(op, "history") == 0)
    {
        char *result = history(executeArgs.history, executeArgs.historyptr,
                               executeArgs.historymutex, true);

        FILE *outfile = fopen(files[*count % 2], "w");

        fprintf(outfile, "%s", result);
        fclose(outfile);
        return;
    }
    if (strcmp(op, "set") == 0)
    {
        char *arg = getcmdinput(First, noargs, files, *count, argsarr);

        if (arg != NULL)
        {
            struct JaggedCharArray varval = splitstr(arg, ' ');
            if (varval.count < 2)
            {
                perror(RED "The set command requires 2 or no arguments" COLOR_RESET);
                return;
            }

            char *var = *varval.arr;
            varval.arr += 1;
            varval.count -= 1;
            char *val = joinarr(varval, ' ', varval.count);

            int result = set(executeArgs.varsDict, executeArgs.varsmutex, var, val);

            FILE *outfile = fopen(files[*count % 2], "w");

            fprintf(outfile, "%d", result);
            fclose(outfile);
        }
        else
        {
            char *result = listvars(executeArgs.varsDict, executeArgs.varsmutex);

            FILE *outfile = fopen(files[*count % 2], "w");

            fprintf(outfile, "%s", result);
            fclose(outfile);
            return;
        }

        return;
    }
    if (strcmp(op, "get") == 0)
    {
        char *arg = getcmdinput(First, noargs, files, *count, argsarr);

        if (arg == NULL)
        {
            perror(RED "The get command requires 1 argument" COLOR_RESET);
            return;
        }

        char *result = get(*executeArgs.varsDict, executeArgs.varsmutex, arg);

        FILE *outfile = fopen(files[*count % 2], "w");

        fprintf(outfile, "%s", result);
        fclose(outfile);
        return;
    }
    if (strcmp(op, "unset") == 0)
    {
        char *arg = getcmdinput(First, noargs, files, *count, argsarr);

        if (arg == NULL)
        {
            perror(RED "The unset command requires 1 argument" COLOR_RESET);
            return;
        }

        int result = unset(executeArgs.varsDict, executeArgs.varsmutex, arg);

        FILE *outfile = fopen(files[*count % 2], "w");

        fprintf(outfile, "%d", result);
        fclose(outfile);
        return;
    }
    if (strcmp(op, "&&") == 0)
    {
        execute_boolean(function, &(*count), files, op, &(*First), executeArgs);
        return;
    }
    if (strcmp(op, "||") == 0)
    {
        execute_boolean(function, &(*count), files, op, &(*First), executeArgs);
        return;
    }
    if (strcmp(op, "true") == 0)
    {
        char command[] = "echo 0";
        struct JaggedCharArray split_command = splitstr(command, ' ');
        execute_pipe(split_command.arr, true, files, 0);
        return;
    }
    if (strcmp(op, "false") == 0)
    {
        char command[] = "echo 1";
        struct JaggedCharArray split_command = splitstr(command, ' ');
        execute_pipe(split_command.arr, true, files, 0);
        return;
    }
    if (strcmp(op, "if") == 0)
    {
        int parenthesis_init = findstr(function, "(");
        int comma_index = -1;
        int parenth_count = 0;
        int len = strlen(function);
        for (int i = parenthesis_init + 1; i < len; i++)
        {
            if (function[i] == ',' && parenth_count == 0)
            {
                comma_index = i;
                break;
            }
            if (function[i] == '(')
                parenth_count++;
            if (function[i] == ')')
                parenth_count--;
        }
        int first_size = comma_index - parenthesis_init - 1;
        char *first = malloc((first_size + 1) * sizeof(char));
        memset(first, 0, (first_size + 1) * sizeof(char));
        memcpy(first, function + parenthesis_init + 1, (first_size) * sizeof(char));

        int cnt = 0;
        main_execute(first, &cnt, if_file, &(*First), executeArgs);
        *First = true;

        // second comma search
        int second_init = parenthesis_init + first_size + 1;
        comma_index = -1;
        parenth_count = 0;
        for (int i = second_init + 1; i < len; i++)
        {
            if (function[i] == ',' && parenth_count == 0)
            {
                comma_index = i;
                break;
            }
            if (function[i] == '(')
                parenth_count++;
            if (function[i] == ')')
                parenth_count--;
        }
        int second_size = comma_index - second_init - 1;
        char *second = malloc((second_size + 1) * sizeof(char));
        memset(second, 0, (second_size + 1) * sizeof(char));
        memcpy(second, function + second_init + 1, (second_size) * sizeof(char));
        // check if if-statment was true
        char *output = malloc(5 * sizeof(char));
        memset(output, 0, 5 * sizeof(char));
        output = read_file(if_file, 0, &(*First));
        if (strcmp(output, "0\n") == 0)
        {
            main_execute(second, &(*count), files, &(*First), executeArgs);
        }

        // third comma search
        int third_init = second_init + second_size + 1;
        int third_size = (len - 1) - third_init - 1;
        if (third_size == 0)
            return;
        char *third = malloc((third_size + 1) * sizeof(char));
        memset(third, 0, (third_size + 1) * sizeof(char));
        memcpy(third, function + third_init + 1, (third_size) * sizeof(char));
        // check if if-statment was true
        if (strcmp(output, "1\n") == 0)
        {
            main_execute(third, &(*count), files, &(*First), executeArgs);
        }
    }
}
void execute_nonboolean(char *function, int *count, char *files[], char *op, bool *First, struct ExecuteArgs executeArgs)
{
    int parenthesis_init = findstr(function, "(");
    int comma_index = -1;
    int parenth_count = 0;
    int len = strlen(function);
    for (int i = parenthesis_init + 1; i < len; i++)
    {
        if (function[i] == ',' && parenth_count == 0)
        {
            comma_index = i;
            break;
        }
        if (function[i] == '(')
            parenth_count++;
        if (function[i] == ')')
            parenth_count--;
    }
    int left_size = comma_index - parenthesis_init - 1;
    char *left = malloc((left_size + 1) * sizeof(char));
    memset(left, 0, (left_size + 1) * sizeof(char));
    memcpy(left, function + parenthesis_init + 1, (left_size) * sizeof(char));
    int right_size = (len - 1) - comma_index - 1;
    char *right = malloc((right_size + 1) * sizeof(char));
    memset(right, 0, (right_size + 1) * sizeof(char));
    memcpy(right, function + comma_index + 1, right_size * sizeof(char));
    main_execute(left, &(*count), files, &(*First), executeArgs);
    if (strcmp(op, "|") == 0)
        (*count)++;

    printf("%i", *count);

    main_execute(right, &(*count), files, &(*First), executeArgs);
    free(right);
    free(left);
    return;
}

void execute_boolean(char *function, int *count, char *files[], char *op, bool *First, struct ExecuteArgs executeArgs)
{
    int parenthesis_init = findstr(function, "(");
    int comma_index = -1;
    int parenth_count = 0;
    int len = strlen(function);
    for (int i = parenthesis_init + 1; i < len; i++)
    {
        if (function[i] == ',' && parenth_count == 0)
        {
            comma_index = i;
            break;
        }
        if (function[i] == '(')
            parenth_count++;
        if (function[i] == ')')
            parenth_count--;
    }
    int left_size = comma_index - parenthesis_init - 1;
    char *left = malloc((left_size + 1) * sizeof(char));
    memset(left, 0, (left_size + 1) * sizeof(char));
    memcpy(left, function + parenthesis_init + 1, (left_size) * sizeof(char));
    int right_size = (len - 1) - comma_index - 1;
    char *right = malloc((right_size + 1) * sizeof(char));
    memset(right, 0, (right_size + 1) * sizeof(char));
    memcpy(right, function + comma_index + 1, right_size * sizeof(char));
    main_execute(left, &(*count), files, &(*First), executeArgs);
    *First = true;
    char *output = malloc(5 * sizeof(char));
    memset(output, 0, 5 * sizeof(char));
    // read the output
    output = read_file(files, *count, &(*First));

    if (strcmp(op, "&&") == 0)
    {
        if (strcmp(output, "1\n") == 0)
        {
            free(right);
            free(left);
            free(output);
            return;
        }
    }
    if (strcmp(op, "||") == 0)
    {
        if (strcmp(output, "0\n") == 0)
        {
            free(right);
            free(left);
            free(output);
            return;
        }
    }
    main_execute(right, &(*count), files, &(*First), executeArgs);
    free(right);
    free(left);
    return;
}
