#ifndef DSTARTOOLS_H
#define DSTARTOOLS_H

#include <stddef.h>
#include <stdint.h>

#define COPY_MAKE_STRING       (1 << 0)
#define COPY_CLEAR_MODULE      (1 << 1)
#define COPY_CONVERT_SSID      (1 << 2)

#define COMPARE_SKIP_MODULE    (1 << 0)

#ifdef __cplusplus
extern "C"
{
#endif

char GetDStarModule(const char* call);
int CopyDStarCall(const char* source, char* destination, char* module, int options);
void CopyDStarString(const char* source, char* destination, size_t length);
char CompareDStarCall(const char* call1, const char* call2, int options);
void SwapDStarCalls(char* call1, char* call2);
uint16_t CalculateCCITTCheckSum(const void* data, size_t length);

#ifdef __cplusplus
};
#endif

#endif
