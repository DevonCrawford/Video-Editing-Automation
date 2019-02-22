#ifndef _UTIL_API_
#define _UTIL_API_

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>

char *catVars(char **dest, int num, ...);

bool safeRealloc(char **data, size_t size);

/**
 * Efficient version of printVarsT, eliminating the array of strings and
 * accessing directly from valist, only one malloc is needed inside this function
 * Function to dynamically allocate and create a string with a variable
 * number of arguments, works similar to printf. (2, str1, str2, ..)
 * @param  num     number of string arguments
 * @param  VARARGS variable number of char * arguments, to be concatenated
 * @return         char * of concatenated string
 */
char *printVars(int num, ...);

/**
 * Clear string with \0 characters
 * @param str char * to be cleared
 * @param num length of string
 */
void clearStr(char *str, int num);

#endif