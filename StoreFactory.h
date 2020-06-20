#ifndef STOREFACTORY_H
#define STOREFACTORY_H

#include "DDBClient.h"

namespace StoreFactory
{
  DDBClient::Store* createStore(const char* type, const char* file, DDBClient::ReportHandler handler);
};

#endif