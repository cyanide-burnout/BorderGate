#include "Dump.h"
#include <stdint.h>
#include <stdlib.h>

#define DUMP_LINE_WIDTH   16

void dump(const char* description, const void* data, size_t length)
{
  size_t index = 0;
  size_t position = 0;
  char buffer[DUMP_LINE_WIDTH + 1];
  const uint8_t* pointer = (const uint8_t*)data;

  printf("%s (%d bytes): ", description, length);

  while (index < length)
  {
    if ((position == 0) && (length > DUMP_LINE_WIDTH))
      printf("\n  %08x ", index);

    printf(" %02x", *pointer);
    buffer[position] = (*pointer > ' ') ? *pointer : '.';

    index ++;
    pointer ++;
    position ++;

    if ((position == DUMP_LINE_WIDTH) || (index == length))
    {
      buffer[position] = '\0';
      printf("  --  %s", buffer);
      position = 0;
    }
  }

  printf("\n");
}
