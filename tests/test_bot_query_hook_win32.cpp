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

static unsigned char mock_sendto_target[16];
static BOOL mock_virtualprotect_result = 1;
static int mock_wsasendto_result = 0;
static DWORD mock_wsasendto_sent = 0;
static int mock_wsa_last_error = 0;
static DWORD mock_last_error_val = 0;

// ============================================================
// Mock Win32 API implementations
// ============================================================

void InitializeCriticalSection(CRITICAL_SECTION *cs) { (void)cs; }
void EnterCriticalSection(CRITICAL_SECTION *cs) { (void)cs; }
void LeaveCriticalSection(CRITICAL_SECTION *cs) { (void)cs; }
void DeleteCriticalSection(CRITICAL_SECTION *cs) { (void)cs; }

HMODULE GetModuleHandle(const char *name)
{ (void)name; return (HMODULE)1; }

FARPROC GetProcAddress(HMODULE module, const char *name)
{ (void)module; (void)name; return (FARPROC)(void *)mock_sendto_target; }

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
   sendto_original = NULL;
   memset(sendto_new_bytes, 0, sizeof(sendto_new_bytes));
   memset(sendto_old_bytes, 0, sizeof(sendto_old_bytes));
   memset(&mutex_replacement_sendto, 0, sizeof(mutex_replacement_sendto));
   memset(mock_sendto_target, 0xCC, sizeof(mock_sendto_target));
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
   TEST("hook_sendto_function success");
   reset_hook_state();

   // Save original bytes to compare after unhook
   unsigned char original_bytes[16];
   memcpy(original_bytes, mock_sendto_target, sizeof(original_bytes));

   ASSERT_TRUE(hook_sendto_function() == true);
   ASSERT_TRUE(is_sendto_hook_setup == true);

   // sendto_original should point to our mock buffer
   ASSERT_TRUE((void *)sendto_original == (void *)mock_sendto_target);

   // Old bytes should contain the original 0xCC pattern
   ASSERT_INT(memcmp(sendto_old_bytes, original_bytes, BYTES_SIZE), 0);

   // mock_sendto_target should now have JMP opcode
   ASSERT_INT(mock_sendto_target[0], 0xe9);

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
   memcpy(original_bytes, mock_sendto_target, sizeof(original_bytes));

   ASSERT_TRUE(hook_sendto_function() == true);
   // mock_sendto_target should now be patched with JMP
   ASSERT_INT(mock_sendto_target[0], 0xe9);

   ASSERT_TRUE(unhook_sendto_function() == true);
   ASSERT_TRUE(is_sendto_hook_setup == false);

   // Bytes should be restored to original 0xCC pattern
   ASSERT_INT(memcmp(mock_sendto_target, original_bytes, BYTES_SIZE), 0);

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
   fail |= test_hook_already_setup();
   fail |= test_hook_virtualprotect_fails();
   fail |= test_unhook_restores_bytes();
   fail |= test_call_original_sendto_success();
   fail |= test_call_original_sendto_failure();

   printf("\n%d/%d tests passed\n", tests_passed, tests_run);
   return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
