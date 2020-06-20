#ifndef STRINGTOOLS_H
#define STRINGTOOLS_H

#include <unistd.h>

#ifdef __cplusplus
extern "C"
{
#endif

char* extract(char** pointer, size_t length);
char* replace(char* string, char search, char replace, ssize_t length);
char* lower(char* string, ssize_t length);
char* upper(char* string, ssize_t length);
char* find(char* string, size_t length, char needle);
void clean(char* string, char replace, size_t length);
void release(char** array);

#ifdef __cplusplus
};
#endif

#endif
