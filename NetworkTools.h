#ifndef NETWORKTOOLS_H
#define NETWORKTOOLS_H

#include <netinet/in.h>

#ifdef __cplusplus
extern "C"
{
#endif

int OpenUDPSocket(const char* address, int port);
int CompareSocketAddress(const struct sockaddr_in* address1, const struct sockaddr_in* address2);
void ParseAddressString(struct sockaddr_in* address, const char* location);

#ifdef __cplusplus
}
#endif

#endif

