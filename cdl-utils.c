#include <dirent.h>
#include "cdl-utils.h"

bool is_valid_directory(char *dir)
{
    DIR *pDir;

    pDir = opendir(dir);
    return pDir != NULL;
}

struct JaggedCharArray splitstr(char *str, char sep)
{
    int count;
    int i;

    for(i = 0; ; i++)
    {
        if(str[i] == '\0')
            break;
        
        if(str[i] == sep)
        {
            
        }
    }
}