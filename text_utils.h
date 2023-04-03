#include <windows.h>
#include <stdbool.h>

#define ANSI_BOLD "\\e[1m"
#define ANSI_BOLD_RESET "\\e[0m"

#define ANSI_RED "\\x1b[31m"
#define ANSI_GREEN "\\x1b[32m"
#define ANSI_YELLOW "\\x1b[33m"
#define ANSI_BLUE "\\x1b[34m"
#define ANSI_CYAN "\\x1b[36m"
#define ANSI_COLOR_RESET "\\x1b[0m"

#define WIN_RED FOREGROUND_RED
#define WIN_GREEN FOREGROUND_GREEN
#define WIN_YELLOW FOREGROUND_RED | FOREGROUND_GREEN
#define WIN_BLUE FOREGROUND_BLUE
#define WIN_CYAN FOREGROUND_GREEN | FOREGROUND_BLUE
#define WIN_BOLD FOREGROUND_INTENSITY

#define BOLD ANSI_BOLD 
#define BOLD_RESET ANSI_BOLD_RESET

#define RED ANSI_RED
#define GREEN ANSI_GREEN 
#define YELLOW ANSI_YELLOW 
#define BLUE ANSI_BLUE 
#define CYAN ANSI_CYAN 
#define COLOR_RESET ANSI_COLOR_RESET

#define COLOR_TOKENS {BOLD, BOLD_RESET, RED, GREEN, YELLOW, BLUE, CYAN, COLOR_RESET}
#define TOKENS_COUNT 8

struct WindowsAttribute
{
    PWORD attribute;
    bool valid;
};

struct WindowsAttribute toWindowsAttribute(char *token, WORD savedAttributes, WORD currentAttributes);