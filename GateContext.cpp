#include "GateContext.h"
#include "NetworkTools.h"
#include "DStarTools.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <netdb.h>

GateContext::GateContext(char module, const char* call, const char* server, int port) :
  module(module),
  state(LINK_STATE_DISCONNECTED),
  option(0),
  number(0),
  time(0)
{
  CopyDStarCall(call, this->call, NULL, COPY_MAKE_STRING);
  this->server = strdup(server);

  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  address.sin_addr.s_addr = htonl(INADDR_NONE);
}

GateContext::~GateContext()
{
  free(server);
}

bool GateContext::matchesTo(const char* call)
{
  return (CompareDStarCall(this->call, call, 0) == 0);
}

bool GateContext::matchesTo(const struct sockaddr_in& address)
{
  return (CompareSocketAddress(&this->address, &address) == 0);
}

void GateContext::touch()
{
  time = ::time(NULL) + LINK_CONNECTION_TIMEOUT;
}

void GateContext::setOption(int value, int mask)
{
  option &= ~mask;
  option |= value;
}

void GateContext::setState(int value)
{
  if ((value >= LINK_STATE_CONNECTING) && (state != value))
    number = 0;

  state = value;
  touch();
}

char GateContext::getModule()
{
  return module;
}

const char* GateContext::getCall()
{
  return call;
}

const char* GateContext::getServer()
{
  return server;
}

const struct sockaddr_in* GateContext::getAddress()
{
  if (state == LINK_STATE_DISCONNECTED)
  {
    struct hostent* entry = gethostbyname(server);
    if ((entry != NULL) && (entry->h_addrtype == AF_INET))
      memcpy(&address.sin_addr, entry->h_addr, entry->h_length);
  }
  return &address;
}

unsigned int GateContext::getFrameNumber()
{
  unsigned int value = number;
  number ++;
  return value;
}

int GateContext::getOption(int mask)
{
  return option & mask;
}

int GateContext::getState()
{
  if ((state != LINK_STATE_DISCONNECTED) && (::time(NULL) > time))
    state = LINK_STATE_DISCONNECTED;
  return state;
}
