#include "Util.h"

char *catVars(char **dest, int num, ...) {
    va_list valist;
    va_start(valist, num);

    size_t len = 1 + strlen(*dest);
    for(int i = 0; i < num; i++) {
        char *tmp = va_arg(valist, char *);
        if(tmp != NULL) {
            len += strlen(tmp);
        }
    }
    if(!safeRealloc(dest, sizeof(char) * len)) {
        return *dest;
    }

    va_start(valist, num);
    for(int i = 0; i < num; i++) {
        char *tmp = va_arg(valist, char *);
        if(tmp != NULL) {
            strcat(*dest, tmp);
        }
    }
    (*dest)[len - 1] = '\0';
    va_end(valist);
    return *dest;
}

bool safeRealloc(char **data, size_t size) {
    char *tmp = realloc(*data, size);

    if(tmp == NULL) {
        return false;
    }
    *data = tmp;
    return true;
}

/**
 * Efficient version of printVarsT, eliminating the array of strings and
 * accessing directly from valist, only one malloc is needed inside this function
 * Function to dynamically allocate and create a string with a variable
 * number of arguments, works similar to printf. (2, str1, str2, ..)
 * @param  num     number of string arguments
 * @param  VARARGS variable number of char * arguments, to be concatenated
 * @return         char * of concatenated string
 */
char *printVars(int num, ...) {
    va_list valist;
    va_start(valist, num);

    size_t len = 1;
    for(int i = 0; i < num; i++) {
        char *tmp = va_arg(valist, char *);
        if(tmp != NULL) {
            len += strlen(tmp);
        }
    }
    char *str = malloc(sizeof(char) * len);
    clearStr(str, len);

    va_start(valist, num);
    for(int i = 0; i < num; i++) {
        char *tmp = va_arg(valist, char *);
        if(tmp != NULL) {
            strcat(str, tmp);
        }
    }
    str[len - 1] = '\0';
    va_end(valist);
    return str;
}

/**
 * Clear string with \0 characters
 * @param str char * to be cleared
 * @param num length of string
 */
void clearStr(char *str, int num) {
    for(int i = 0; i < num; i++) {
        str[i] = 0;
    }
}