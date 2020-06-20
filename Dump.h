#ifndef DUMP_H
#define DUMP_H

#include <stddef.h>
#include <stdio.h>

// http://www.linuxquestions.org/questions/linux-general-1/shell-color-codes-48172/
#define DEFAULT_COLOR "\x1b[m"
#define RED_COLOR     "\x1b[01;31m"
#define GREEN_COLOR   "\x1b[01;32m"
#define YELLOW_COLOR  "\x1b[01;33m"
#define BLUE_COLOR    "\x1b[01;34m"
#define MAGENTA_COLOR "\x1b[01;35m"
#define CYAN_COLOR    "\x1b[01;36m"
#define WHITE_COLOR   "\x1b[01;37m"

#ifdef __cplusplus
extern "C"
{
#endif

void dump(const char* description, const void* data, size_t length);

#ifdef __cplusplus
};
#endif

#endif
