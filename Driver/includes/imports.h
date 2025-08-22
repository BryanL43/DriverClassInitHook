#ifndef IMPORTS_H
#define IMPORTS_H

#include <wdm.h>

#if defined(_WIN64)
#define EPROCESS_UNIQUEPID_OFFSET 0x440
#define EPROCESS_ACTIVELINKS_OFFSET 0x448
#define EPROCESS_THREADS_OFFSET 0x5F0
#else
#error "This driver only supports 64-bit Windows (_WIN64 must be defined)"
#endif

typedef struct _PEB *(__cdecl *pfnPsGetProcessPeb)(struct _KPROCESS *);
extern pfnPsGetProcessPeb PsGetProcessPeb;

NTSTATUS InitializeFunctionPointers();

#endif
