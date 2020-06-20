#include "StringTools.h"
#include <ctype.h>
#include <stdlib.h>

char* extract(char** pointer, size_t length)
{
  char* string = *pointer;
  (*pointer)[length] = '\0';
  (*pointer) += length + 1;
  return string;
}

char* replace(char* string, char search, char replace, ssize_t length)
{
  char* pointer = string;
  while ((*pointer != '\0') && (length != 0))
  {
    if (*pointer == search)
      *pointer = replace;
    pointer ++;
    length --;
  }
  return string;
}

char* lower(char* string, ssize_t length)
{
  char* pointer = string;
  while ((*pointer != '\0') && (length != 0))
  {
    *pointer = tolower(*pointer);
    pointer ++;
    length --;
  }
  return string;
}

char* upper(char* string, ssize_t length)
{
  char* pointer = string;
  while ((*pointer != '\0') && (length != 0))
  {
    *pointer = toupper(*pointer);
    pointer ++;
    length --;
  }
  return string;
}

char* find(char* string, size_t length, char needle)
{
  while ((*string != '\0') && (length != 0))
  {
    if (*string == needle)
      return string;
    string ++;
    length --;
  }
  return NULL;
}

void clean(char* string, char replace, size_t length)
{
  while (length > 0)
  {
    if (*string <= ' ')
      *string = replace;
    string ++;
    length --;
  }
}

void release(char** array)
{
  char** string = array;
  if (array != NULL)
  {
    while (*string != NULL)
    {
      free(*string);
      string ++;
    }
    free(array);
  }
}
