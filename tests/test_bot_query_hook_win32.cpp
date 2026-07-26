//
// JK_Botti - tests for bot_query_hook_win32.cpp
//
// test_bot_query_hook_win32.cpp
//
// Compiles the win32 hook source on Linux with mocked Win32 API.
// Compile with: -D_WIN32 -I. (tests dir for mock windows.h/winsock2.h)
//

#ifndef _WIN32
#define _WIN32
#endif

#include <stdlib.h>
#include <errno.h>

#include "../bot_query_hook_win32.cpp"

#include <string.h>

#include "test_common.h"

// ============================================================
// Mock state
// ============================================================

// Distinct backing buffers for each DLL's "sendto". 8-byte aligned so the
// locked cmpxchg8b publish stays within one cache line (no split lock).
static unsigned char mock_ws2_target[16] __attribute__((aligned(8)));
static unsigned char mock_wsock_target[16] __attribute__((aligned(8)));

// Per-DLL knobs: whether the module is loaded, and what address its "sendto"
// resolves to (NULL == GetProcAddress fails). Default mirrors wine: both DLLs
// loaded, wsock32's export forwarding to the same address as ws2_32.
static bool mock_ws2_loaded = true;
static bool mock_wsock_loaded = true;
static void *mock_ws2_sendto = mock_ws2_target;
static void *mock_wsock_sendto = mock_ws2_target;

static BOOL mock_virtualprotect_result = 1;
static int mock_wsasendto_result = 0;
static DWORD mock_wsasendto_sent = 0;
static int mock_wsa_last_error = 0;
static DWORD mock_last_error_val = 0;

#define MOCK_H_WS2   ((HMODULE)0x2001)
#define MOCK_H_WSOCK ((HMODULE)0x2002)

// ============================================================
// Mock Win32 API implementations
// ============================================================

void InitializeCriticalSection(CRITICAL_SECTION *cs) { (void)cs; }
void EnterCriticalSection(CRITICAL_SECTION *cs) { (void)cs; }
void LeaveCriticalSection(CRITICAL_SECTION *cs) { (void)cs; }
void DeleteCriticalSection(CRITICAL_SECTION *cs) { (void)cs; }

HMODULE GetModuleHandle(const char *name)
{
   if (name && strcmp(name, "ws2_32.dll") == 0)
      return mock_ws2_loaded ? MOCK_H_WS2 : (HMODULE)NULL;
   if (name && strcmp(name, "wsock32.dll") == 0)
      return mock_wsock_loaded ? MOCK_H_WSOCK : (HMODULE)NULL;
   return (HMODULE)NULL;
}

FARPROC GetProcAddress(HMODULE module, const char *name)
{
   (void)name;
   if (module == MOCK_H_WS2)
      return (FARPROC)mock_ws2_sendto;
   if (module == MOCK_H_WSOCK)
      return (FARPROC)mock_wsock_sendto;
   return (FARPROC)NULL;
}

BOOL VirtualProtect(void *addr, size_t size, DWORD newprotect, DWORD *oldprotect)
{
   (void)addr; (void)size; (void)newprotect;
   if (oldprotect) *oldprotect = 0;
   return mock_virtualprotect_result;
}

DWORD GetLastError(void) { return mock_last_error_val; }

int WSASendTo(int s, WSABUF *lpBuffers, DWORD dwBufferCount,
              DWORD *lpNumberOfBytesSent, DWORD dwFlags,
              const struct sockaddr *lpTo, int iToLen,
              void *lpOverlapped, void *lpCompletionRoutine)
{
   (void)s; (void)lpBuffers; (void)dwBufferCount; (void)dwFlags;
   (void)lpTo; (void)iToLen; (void)lpOverlapped; (void)lpCompletionRoutine;
   if (lpNumberOfBytesSent) *lpNumberOfBytesSent = mock_wsasendto_sent;
   return mock_wsasendto_result;
}

int WSAGetLastError(void) { return mock_wsa_last_error; }

// ============================================================
// Stubs for bot_query_hook.h declared functions
// ============================================================

int bot_conntimes = 0;

void UTIL_ConsolePrintf(const char *fmt, ...) { (void)fmt; }

void BotReplaceConnectionTime(const char *name, float *timeslot)
{ (void)name; (void)timeslot; }

ssize_t PASCAL sendto_hook(int socket, const void *message, size_t length,
                           int flags, const struct sockaddr *dest_addr,
                           socklen_t dest_len)
{
   (void)socket; (void)message; (void)flags;
   (void)dest_addr; (void)dest_len;
   return (ssize_t)length;
}

// ============================================================
// Helper
// ============================================================

static void reset_hook_state(void)
{
   is_sendto_hook_setup = false;
   num_sendto_hooks = 0;
   memset(sendto_hooks, 0, sizeof(sendto_hooks));
   memset(&mutex_replacement_sendto, 0, sizeof(mutex_replacement_sendto));
   memset(mock_ws2_target, 0xCC, sizeof(mock_ws2_target));
   memset(mock_wsock_target, 0xCC, sizeof(mock_wsock_target));
   // Default: both DLLs loaded, wsock32 forwards to ws2_32 (wine-like) -> dedup.
   mock_ws2_loaded = true;
   mock_wsock_loaded = true;
   mock_ws2_sendto = mock_ws2_target;
   mock_wsock_sendto = mock_ws2_target;
   mock_virtualprotect_result = 1;
   mock_wsasendto_result = 0;
   mock_wsasendto_sent = 0;
   mock_wsa_last_error = 0;
   mock_last_error_val = 0;
}

// ============================================================
// Tests
// ============================================================

static int test_unhook_when_not_hooked(void)
{
   TEST("unhook when not hooked -> true");
   reset_hook_state();

   ASSERT_TRUE(unhook_sendto_function() == true);
   ASSERT_TRUE(is_sendto_hook_setup == false);

   PASS();
   return 0;
}

static int test_hook_success(void)
{
   TEST("hook_sendto_function success (shared addr -> 1 hook)");
   reset_hook_state();

   // Save original bytes to compare after unhook
   unsigned char original_bytes[16];
   memcpy(original_bytes, mock_ws2_target, sizeof(original_bytes));

   ASSERT_TRUE(hook_sendto_function() == true);
   ASSERT_TRUE(is_sendto_hook_setup == true);

   // wsock32 forwards to ws2_32 -> deduped to a single hook site
   ASSERT_INT(num_sendto_hooks, 1);
   ASSERT_TRUE((void *)sendto_hooks[0].original == (void *)mock_ws2_target);

   // Old bytes should contain the original 0xCC pattern
   ASSERT_INT(memcmp(sendto_hooks[0].old_bytes, original_bytes, BYTES_SIZE), 0);

   // target should now have JMP opcode
   ASSERT_INT(mock_ws2_target[0], 0xe9);

   PASS();
   return 0;
}

static int test_hook_distinct_addrs(void)
{
   TEST("hook both when ws2_32/wsock32 differ -> 2 hooks");
   reset_hook_state();
   // wsock32's sendto is a distinct implementation (real Windows)
   mock_wsock_sendto = mock_wsock_target;

   ASSERT_TRUE(hook_sendto_function() == true);
   ASSERT_INT(num_sendto_hooks, 2);

   // Both patch sites written with JMP
   ASSERT_INT(mock_ws2_target[0], 0xe9);
   ASSERT_INT(mock_wsock_target[0], 0xe9);

   // Both restored on unhook
   ASSERT_TRUE(unhook_sendto_function() == true);
   ASSERT_INT(mock_ws2_target[0], 0xCC);
   ASSERT_INT(mock_wsock_target[0], 0xCC);

   PASS();
   return 0;
}

static int test_hook_only_ws2(void)
{
   TEST("hook when only ws2_32 loaded -> 1 hook on ws2_32");
   reset_hook_state();
   mock_wsock_loaded = false;

   ASSERT_TRUE(hook_sendto_function() == true);
   ASSERT_INT(num_sendto_hooks, 1);
   ASSERT_TRUE((void *)sendto_hooks[0].original == (void *)mock_ws2_target);
   ASSERT_INT(mock_ws2_target[0], 0xe9);

   PASS();
   return 0;
}

static int test_hook_only_wsock(void)
{
   TEST("hook when only wsock32 loaded -> 1 hook on wsock32");
   reset_hook_state();
   mock_ws2_loaded = false;
   mock_wsock_sendto = mock_wsock_target;

   ASSERT_TRUE(hook_sendto_function() == true);
   ASSERT_INT(num_sendto_hooks, 1);
   ASSERT_TRUE((void *)sendto_hooks[0].original == (void *)mock_wsock_target);
   ASSERT_INT(mock_wsock_target[0], 0xe9);

   PASS();
   return 0;
}

// Reproduces issue #126: neither DLL exposes sendto -> must fail, not crash.
static int test_hook_none_loaded(void)
{
   TEST("hook when no sendto found -> false, no crash");
   reset_hook_state();
   mock_ws2_loaded = false;
   mock_wsock_loaded = false;

   ASSERT_TRUE(hook_sendto_function() == false);
   ASSERT_TRUE(is_sendto_hook_setup == false);
   ASSERT_INT(num_sendto_hooks, 0);

   PASS();
   return 0;
}

static int test_hook_getprocaddress_null(void)
{
   TEST("hook when GetProcAddress returns NULL -> false, no crash");
   reset_hook_state();
   // Both modules loaded but neither exports sendto
   mock_ws2_sendto = NULL;
   mock_wsock_sendto = NULL;

   ASSERT_TRUE(hook_sendto_function() == false);
   ASSERT_TRUE(is_sendto_hook_setup == false);
   ASSERT_INT(num_sendto_hooks, 0);

   PASS();
   return 0;
}

static int test_hook_already_setup(void)
{
   TEST("hook when already hooked -> true immediately");
   reset_hook_state();

   ASSERT_TRUE(hook_sendto_function() == true);
   ASSERT_TRUE(is_sendto_hook_setup == true);

   // Second call should return true without re-hooking
   ASSERT_TRUE(hook_sendto_function() == true);

   PASS();
   return 0;
}

static int test_hook_virtualprotect_fails(void)
{
   TEST("hook VirtualProtect fails -> false");
   reset_hook_state();
   mock_virtualprotect_result = 0; // FALSE
   mock_last_error_val = 42;

   ASSERT_TRUE(hook_sendto_function() == false);
   ASSERT_TRUE(is_sendto_hook_setup == false);

   PASS();
   return 0;
}

static int test_unhook_restores_bytes(void)
{
   TEST("unhook restores original bytes");
   reset_hook_state();

   unsigned char original_bytes[16];
   memcpy(original_bytes, mock_ws2_target, sizeof(original_bytes));

   ASSERT_TRUE(hook_sendto_function() == true);
   // target should now be patched with JMP
   ASSERT_INT(mock_ws2_target[0], 0xe9);

   ASSERT_TRUE(unhook_sendto_function() == true);
   ASSERT_TRUE(is_sendto_hook_setup == false);

   // Bytes should be restored to original 0xCC pattern
   ASSERT_INT(memcmp(mock_ws2_target, original_bytes, BYTES_SIZE), 0);

   PASS();
   return 0;
}

static int test_call_original_sendto_success(void)
{
   TEST("call_original_sendto success");
   reset_hook_state();
   mock_wsasendto_result = 0;
   mock_wsasendto_sent = 42;

   unsigned char buf[] = "hello";
   ssize_t ret = call_original_sendto(0, buf, sizeof(buf), 0, NULL, 0);
   ASSERT_INT((int)ret, 42);

   PASS();
   return 0;
}

static int test_call_original_sendto_failure(void)
{
   TEST("call_original_sendto SOCKET_ERROR -> -1");
   reset_hook_state();
   mock_wsasendto_result = SOCKET_ERROR;
   mock_wsa_last_error = 10054;

   unsigned char buf[] = "hello";
   ssize_t ret = call_original_sendto(0, buf, sizeof(buf), 0, NULL, 0);
   ASSERT_INT((int)ret, -1);
   ASSERT_INT(errno, 10054);

   PASS();
   return 0;
}

// ============================================================
// main
// ============================================================

int main(void)
{
   int fail = 0;

   printf("test_bot_query_hook_win32:\n");
   fail |= test_unhook_when_not_hooked();
   fail |= test_hook_success();
   fail |= test_hook_distinct_addrs();
   fail |= test_hook_only_ws2();
   fail |= test_hook_only_wsock();
   fail |= test_hook_none_loaded();
   fail |= test_hook_getprocaddress_null();
   fail |= test_hook_already_setup();
   fail |= test_hook_virtualprotect_fails();
   fail |= test_unhook_restores_bytes();
   fail |= test_call_original_sendto_success();
   fail |= test_call_original_sendto_failure();

   printf("\n%d/%d tests passed\n", tests_passed, tests_run);
   return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
