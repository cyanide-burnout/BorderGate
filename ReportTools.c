#include "ReportTools.h"

#include <unistd.h>
#include <syslog.h>
#include <stdarg.h>
#include <stdio.h>

#include <curses.h>
#include <term.h>

#define DEFAULT_COLOR "\x1b[m"
#define RED_COLOR     "\x1b[01;31m"
#define GREEN_COLOR   "\x1b[01;32m"
#define YELLOW_COLOR  "\x1b[01;33m"
#define BLUE_COLOR    "\x1b[01;34m"
#define MAGENTA_COLOR "\x1b[01;35m"
#define CYAN_COLOR    "\x1b[01;36m"
#define WHITE_COLOR   "\x1b[01;37m"

int reportOption = 0;

void doReport(int priority, const char* format, va_list arguments)
{
  if (reportOption & LOGGER_OUTPUT_SYSLOG)
    vsyslog(priority, format, arguments);

  if (reportOption & LOGGER_OUTPUT_CONSOLE)
  {
    if (reportOption & LOGGER_CONSOLE_COLOR)
    {
      priority &= 7;
      const char* priorities[] =
      {
        RED_COLOR,      // Emergency
        RED_COLOR,      // Alert
        RED_COLOR,      // Critical
        YELLOW_COLOR,   // Error
        MAGENTA_COLOR,  // Warning
        WHITE_COLOR,    // Notice
        GREEN_COLOR,    // Informational
        CYAN_COLOR      // Debug
      };
      printf(priorities[priority]);
    }
    vprintf(format, arguments);
    printf(DEFAULT_COLOR);
  }
}

void setReportOutput(int option)
{
  if (option & LOGGER_OUTPUT_CONSOLE)
  {
    int error = 0;
    if ((setupterm(NULL, 1, &error) != ERR) &&
        (error == 0) &&
        (has_colors() != 0))
      option |= LOGGER_CONSOLE_COLOR;
  }
  reportOption = option;
}

void checkReportOutput()
{
  if (reportOption == 0)
    setReportOutput(LOGGER_OUTPUT_CONSOLE);
}

void report(int priority, const char* format, ...)
{
  checkReportOutput();
  va_list arguments;
  va_start(arguments, format);
  doReport(priority, format, arguments);
  va_end(arguments);
}

void print(const char* format, ...)
{
  checkReportOutput();
  va_list arguments;
  va_start(arguments, format);
  doReport(LOG_INFO, format, arguments);
  va_end(arguments);
}
