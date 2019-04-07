#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * Expands environment variables (identified by $ prefix, e.g., $SHELL) in a
 * string by resizing and inserting the value of the variable into the string.
 * This function only does one expansion, but can be called multiple times on a
 * string to expand more than one variable.
 * 
 * NOTE: this function allocates memory! The caller is responsible for freeing
 * the memory. 
 *
 * Parameters:
 * - str: The string with variables to expand
 *
 * Returns: char pointer to the newly-expanded and allocated string. Returns
 * NULL if there are no variables to replace or if memory cannot be allocated.
 */
char *expand_var(char *str)
{
    size_t var_start = 0;
    var_start = strcspn(str, "$");
    if (var_start == strlen(str)) {
        /* No variable to replace */
        return NULL;
    }

    size_t var_len = strcspn(str + var_start, " \t\r\n\'\"");

    char *var_name = malloc(sizeof(char) * var_len + 1);
    if (var_name == NULL) {
        return NULL;
    }
    strncpy(var_name, str + var_start, var_len);
    var_name[var_len] = '\0';

    if (strlen(var_name) <= 1) {
        free(var_name);
        return NULL;
    }

    /* Use index 1 to ignore the '$' prefix */
    char *value = getenv(&var_name[1]);
    if (value == NULL) {
        fprintf(stderr, "value was null\n");
        value = "";
    }

    fprintf(stderr, "Replacing variable: %s='%s'\n", var_name, value);
    free(var_name);

    /* Grab the size of the remaining string (after the $var): */
    size_t remain_sz = strlen(str + var_start + var_len);

    /* Our final string contains the prefix data, the value of the variable, the
     * remaining string size, and an extra character for the NUL byte. */
    size_t newstr_sz = var_start + strlen(value) + remain_sz + 1;

    char *newstr = malloc(sizeof(char) * newstr_sz);
    if (newstr == NULL) {
        return NULL;
    }

    strncpy(newstr, str, var_start);
    newstr[var_start] = '\0';
    strcat(newstr, value);
    strcat(newstr, str + var_start + var_len);

    return newstr;
}

