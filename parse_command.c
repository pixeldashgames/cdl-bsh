#include "cdl-utils.h"

// the parse has the foolowing structure =>  op(destination,source)
static char *ParsePipes(char *func, int len);
static char *ParseFlow(char *func, int len);
static char *ParseFunction(char *func, int len);
char *ParseCommand(char *func, int len);
int main()
{
    char func[MAX_COMMAND_LENGTH] = "command1 arg11 arg12 < file1 | command2 arg21";
    char *f = func;
    f = ParseCommand(f, strlen(f));
    printf("%s \n", f);
}
// This is the main method
char *ParseCommand(char *func, int len)
{
    return ParsePipes(func, len);
}
static char *ParsePipes(char *func, int len)
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
        return ParseFlow(func, len);
    return func;
}
static char *ParseFlow(char *func, int len)
{
    bool inside = false;
    for (int j = (len - 1); j >= 0; j--)
    {
        if ((func[j] == '<') || (func[j] == '>'))
        {
            inside = true;
            int left_size = j - 1;
            int right_size = len - j - 2;
            char right[MAX_COMMAND_LENGTH];
            char left[MAX_COMMAND_LENGTH];
            char *r = right;
            char *l = left;
            r = ParseFunction(r, right_size);
            strncpy(r, func + j + 2, right_size);
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
            {
                strcat(temp, ">");
                strcat(temp, "(");
                strcat(temp, r);
                strcat(temp, ",");
                strcat(temp, l);
                strcat(temp, ")");
            }
            else
            {
                strcat(temp, "<");
                strcat(temp, "(");
                strcat(temp, l);
                strcat(temp, ",");
                strcat(temp, r);
                strcat(temp, ")");
            }
            func = temp;
            break;
        }
    }
    if (!inside)
        return ParseFunction(func, len);
    return func;
}
char *ParseFunction(char *func, int len)
{
    char temp[len + 1];
    char *t = temp;
    // Searching op
    int op_index = 0;
    for (size_t i = 0; i < len; i++)
    {
        if (func[i] == ' ')
            break;
        else
            op_index = i;
    }
    char operator[op_index + 1];
    // memset(operator, 0, sizeof operator);
    strncpy(operator, func + 0, op_index + 1);
    strncat(t, operator+ 0, op_index + 1);
    strcat(t, "(");
    int last_index = op_index + 2;
    bool have_param = false;
    bool first_param = true;
    for (size_t i = op_index + 2; i < len; i++)
    {
        have_param = true;

        if (func[i] == ' ')
        {
            char arg[i - last_index];
            char *a = arg;
            strncpy(a, func + last_index, i - last_index);
            if (first_param)
                first_param = false;
            else
                strcat(t, ",");
            strncat(t, a, i - last_index);
            last_index = i + 1;
        }
    }
    if (have_param)
    {
        char arg[len - last_index];
        char *a = arg;
        if (!first_param)
            strcat(t, ",");
        strncpy(a, func + last_index, len);
        strcat(t, a);
    }
    strcat(t, ")");
    return t;
}