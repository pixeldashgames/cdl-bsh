#include <stdbool.h>
#include <string.h>
#include "cdl-utils.h"

// the parse has the foolowing structure =>  op(destination,source)
char *ParsePipes(char *func, int len);
char *ParseFlow(char *func, int len);
char *ParseCommand(char *func, int len);
// This is the main method
char *ParseCommand(char *func, int len)
{
    return ParsePipes(func, len);
}
char *ParsePipes(char *func, int len)
{
    char temp[MAX_COMMAND_LENGTH];
    char *t = temp;
    bool inside = false;
    for (int i = (len - 1); i >= 0; i--)
    {
        if (func[i] == '|')
        {
            inside = true;
            int left_size = i - 1;
            int right_size = len - i - 2;
            char right[MAX_COMMAND_LENGTH];
            char left[MAX_COMMAND_LENGTH];
            char *r = right;
            char *l = left;
            strncpy(right, func + i + 2, right_size);
            strncpy(left, func + 0, left_size);
            r = ParseFlow(right, right_size);
            l = ParsePipes(left, left_size);
            strcat(t, "|");
            strcat(t, "(");
            strcat(t, l);
            strcat(t, ",");
            strcat(t, r);
            strcat(t, ")");
            func = t;
            break;
        }
    }
    if (!inside)
        func = ParseFlow(func, len);
    return func;
}
char *ParseFlow(char *func, int len)
{
    for (int j = (len - 1); j >= 0; j--)
    {
        if ((func[j] == '<') || (func[j] == '>'))
        {
            int left_size = j - 1;
            int right_size = len - j - 2;
            char right[MAX_COMMAND_LENGTH];
            char left[MAX_COMMAND_LENGTH];
            char *r = right;
            char *l = left;
            strncpy(right, func + j + 2, right_size);
            int _double = false;
            // this happend when >> is founded
            if (func[j - 1] == '>')
            {
                _double = true;
                left_size -= 1;
                strncpy(left, func + 0, left_size);
            }
            else
            {
                strncpy(left, func + 0, left_size);
            }
            l = ParseFlow(left, left_size);
            char temp[MAX_COMMAND_LENGTH];
            if (_double)
            {
                strcat(temp, ">");
            }
            if (func[j] == '>')
                strcat(temp, ">");
            else
                strcat(temp, "<");
            strcat(temp, "(");
            strcat(temp, l);
            strcat(temp, ",");
            strcat(temp, r);
            strcat(temp, ")");
            func = temp;
            break;
        }
    }
    return func;
}