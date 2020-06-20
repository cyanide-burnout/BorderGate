#include "NetworkTools.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>

int OpenUDPSocket(const char* address, int port)
{
  struct sockaddr_in information;
  int handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  if (handle != EOF)
  {
    information.sin_family = AF_INET;
    information.sin_port = htons(port);
    information.sin_addr.s_addr = htonl(INADDR_ANY);

    if ((address != NULL) && (strcmp(address, "*") != 0))
      inet_aton(address, &information.sin_addr);

    if (bind(handle, (struct sockaddr*)&information, sizeof(information)) == EOF)
    {
      close(handle);
      handle = EOF;
    }
  }

  return handle;
}

int CompareSocketAddress(const struct sockaddr_in* address1, const struct sockaddr_in* address2)
{
  return
    (address1->sin_port != address2->sin_port) ||
    (address1->sin_addr.s_addr != address2->sin_addr.s_addr);
}

void ParseAddressString(struct sockaddr_in* address, const char* location)
{
  char* host = strtok((char*)location, ": ");
  char* port = strtok(NULL, ": ");

  if ((host != NULL) && (port != NULL))
  {
    address->sin_family = AF_INET;
    address->sin_port = htons(atoi(port));
    inet_aton(host, &address->sin_addr);
  }
}