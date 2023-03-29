#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include "text_utils.h"
#include <windows.h>

#define MAX_COMMAND_LENGTH 1024

struct JaggedCharArray
{
    char **arr;
    int count;
};

int main()
{
    // To store the last command entered by the user.
    char cmd[MAX_COMMAND_LENGTH];

    char currentDir[PATH_MAX];
    if (getcwd(currentDir, sizeof(currentDir)) == NULL)
    {
        perror("Error acquiring current working directory. Exiting...");
        return 1;
    }

    while (true)
    {
        char toPrint[MAX_COMMAND_LENGTH];
        sprintf(toPrint, YELLOW "cdl-bsh" COLOR_RESET " - " CYAN "%s" YELLOW BOLD " $ " BOLD_RESET COLOR_RESET, currentDir);
        print(toPrint);
        scanf("%s", &cmd);
        
        return 0;
    }

    return 0;
}

void print(char str[])
{
#ifdef _WIN32
    struct JaggedCharArray tokens = extract_color_tokens(str);

    HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
    WORD savedAttributes;
    GetConsoleScreenBufferInfo(consoleHandle, &consoleInfo);
    savedAttributes = consoleInfo.wAttributes;

    WORD currentAttributes = savedAttributes;

    int i;
    for (i = 0; i < tokens.count; i++)
    {
        print_windows_token(tokens.arr[i], consoleHandle, savedAttributes, &currentAttributes);
        free(tokens.arr[i]);
    }
    free(tokens.arr);
    
#else
    printf(&str);
#endif
}

#ifdef _WIN32
void print_windows_token(char *token, HANDLE consoleHandle, WORD savedAttributes, WORD *currentAttributes)
{
    WORD attribute = toWindowsAttribute(token, savedAttributes, *currentAttributes);

    if (attribute != NULL)
    {
        *currentAttributes = attribute;
        SetConsoleTextAttribute(consoleHandle, attribute);
    }
    else
        printf(&token);
}
#endif

struct JaggedCharArray extract_color_tokens(char str[])
{
    int i;
    int k;
    size_t len = strlen(&str);

    int substringPointer = 0;

    char *parts[len];
    int partIndex = 0;

    char *tokens[] = COLOR_TOKENS;
    int tokensLengths[TOKENS_COUNT];

    bool matchingTokens[TOKENS_COUNT];

    for (i = 0; i < TOKENS_COUNT; i++)
    {
        matchingTokens[i] = false;
        tokensLengths[i] = strlen(tokens[i]);
    }
    
    int falseCount = 0;
    int q = 0;

    for (i = 0; i < len; i++)
    {
        if (q > 0)
        {
            for (k = 0; k < TOKENS_COUNT; k++)
                if(matchingTokens[k] && tokens[k][q] != str[i])
                {
                    matchingTokens[k] = false;
                    falseCount++;
                }
            if (falseCount == TOKENS_COUNT)
            {
                falseCount = 0;
                q = 0;
            }
        }
        
        if (q == 0 && str[i] == '\\') // works because every ANSI color code starts with '\'
        {
            q = 1;
            for (k = 0; k < TOKENS_COUNT; k++)
                matchingTokens[k] = true;
        }
        else if (q > 1) // if it was not reset (or just created), some tokens must have matched
            q++;

        if (q == 0)
            continue;

        for (k = 0; k < TOKENS_COUNT; k++)
            if (matchingTokens[k] && q == tokensLengths[k]) // a token was completely matched
            {
                int preTokenLength = i - q - substringPointer + 1;
                char *preToken = malloc(preTokenLength + 1);
                char *token = malloc(q + 1);

                memcpy(preToken, &str + substringPointer, preTokenLength);
                preToken[preTokenLength] = '\\0';
                memcpy(token, &str + substringPointer + preTokenLength, q);
                token[q] = '\\0';

                parts[partIndex] = preToken;
                parts[partIndex + 1] = token;

                partIndex += 2;
                substringPointer = i + 1;
                q = 0;
                falseCount = 0;

                break;
            }
    }
    // adding the remainder to the array
    if (substringPointer < len)
    {
        int length = len - substringPointer;
        char *substring = malloc(length + 1);

        memcpy(substring, &str + substringPointer, length);
        substring[length] = '\\0';

        parts[partIndex] = substring;
        partIndex++;
    }

    char **ret = malloc(partIndex * sizeof(char*)); 
    memcpy(ret, parts, partIndex);

    struct JaggedCharArray result = { ret, partIndex };
    return result;
}

