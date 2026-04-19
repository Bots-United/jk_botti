// Mock windows.h for testing Win32 code paths on Linux
#ifndef MOCK_WINDOWS_H
#define MOCK_WINDOWS_H

#include <stddef.h>

#define PASCAL
#define WINAPI
#define PAGE_READWRITE 0x04

#ifndef _MOCK_DWORD_DEFINED
#define _MOCK_DWORD_DEFINED
typedef unsigned long DWORD;
#endif

typedef int BOOL;
typedef void *HMODULE;
typedef void (*FARPROC)(void);

typedef struct { int dummy; } CRITICAL_SECTION;

void InitializeCriticalSection(CRITICAL_SECTION *cs);
void EnterCriticalSection(CRITICAL_SECTION *cs);
void LeaveCriticalSection(CRITICAL_SECTION *cs);
void DeleteCriticalSection(CRITICAL_SECTION *cs);
HMODULE GetModuleHandle(const char *name);
FARPROC GetProcAddress(HMODULE module, const char *name);
BOOL VirtualProtect(void *addr, size_t size, DWORD newprotect, DWORD *oldprotect);
DWORD GetLastError(void);
HMODULE LoadLibraryA(const char *name);
BOOL FreeLibrary(HMODULE module);
void OutputDebugStringA(const char *msg);

#endif // MOCK_WINDOWS_H
