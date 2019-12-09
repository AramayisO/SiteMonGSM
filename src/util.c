#include "util.h"
#include <ctype.h>

/******************************************************************************
 *
 * Function:    has_alphanumeric()
 *
 * Description: Checks if a string contains any alphanumeric characters.
 *
 * Returns:     1 if the string contains an alphanumeric character, 0 otherwise. 
 *******************************************************************************/
int has_alphanumeric(const char *str)
{
    int has_alnum = 0;
    while (*str != '\0')
    {
        if (isalnum(*str))
        {
            has_alnum = 1;
            break;
        }
    }
    return has_alnum;
}

