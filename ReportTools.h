#ifndef REPORTTOOLS_H
#define REPORTTOOLS_H

#include <syslog.h>
#include <stdarg.h>

#define LOGGER_OUTPUT_CONSOLE  (1 << 0)
#define LOGGER_OUTPUT_SYSLOG   (1 << 1)
#define LOGGER_CONSOLE_COLOR   (1 << 2)

#ifdef __cplusplus
extern "C"
{
#endif

void setReportOutput(int option);
void doReport(int priority, const char* format, va_list arguments);
void report(int priority, const char* format, ...);
void print(const char* format, ...);

#ifdef __cplusplus
}
#endif

#endif