#include "cdl-utils.c"

// the parse has the foolowing structure =>  op(destination,source)
char *ParsePipes(struct JaggedCharArray func_split, int count);
char *ParseFlow(char *func);
char *ParseFunction(char *func, int len);
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
    struct JaggedCharArray func_split = splitstr(func, '|');
    return ParsePipes(func_split, func_split.count);
}
char *ParsePipes(struct JaggedCharArray func_split, int count)
{
    char *temp = malloc(MAX_COMMAND_LENGTH * sizeof(char));
    if (temp == NULL)
        return NULL;
    memset(temp, 0, MAX_COMMAND_LENGTH);
    char *p_temp = temp;
    // Implementing splitstr Leo's Method
    // If (count == 1) => There is not a | in func
    if (count > 1)
    {
        strcat(p_temp, "|(");
        char *right = ParseFlow(func_split.arr[count - 1]);
        int right_len = strlen(right);
        if (right_len > MAX_COMMAND_LENGTH - 3)
        {
            right_len = MAX_COMMAND_LENGTH - 3;
        }
        strncat(p_temp, right, right_len);
        strcat(temp, ",");
        char *left = ParsePipes(func_split, count - 1);
        int left_len = strlen(left);
        if (left_len > MAX_COMMAND_LENGTH - right_len - 4)
        {
            left_len = MAX_COMMAND_LENGTH - right_len - 4;
        }
        strncat(temp, left, left_len);
        strcat(temp, ")");
        free(right);
        free(left);
        return temp;
    }
    return ParseFlow(func_split.arr[0]);
}
char *ParseFlow(char *func)
{
    bool inside = false;
    int len = strlen(func);
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
            l = ParseFlow(left);
            char temp[MAX_COMMAND_LENGTH];
            if (_double)
            {
                strcat(temp, ">");
            }
            if (func[j] == '>')
            {
                strcat(temp, ">(");
                strncat(temp, r, right_size);
                strcat(temp, ",");
                strncat(temp, l, left_size);
                strcat(temp, ")");
            }
            else
            {
                strcat(temp, "<(");
                strncat(temp, l, left_size);
                strcat(temp, ",");
                strncat(temp, r, right_size);
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