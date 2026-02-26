//
// JK_Botti - placeholder tests for bot_query_hook_linux.cpp
//
// test_bot_query_hook_linux.cpp
//

#include <stdlib.h>
#include <string.h>

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>

#include "bot.h"
#include "bot_query_hook.h"

#include "engine_mock.h"
#include "test_common.h"

// ============================================================
// Stubs for symbols not in engine_mock
// ============================================================

int bot_conntimes = 0;

void BotReplaceConnectionTime(const char *name, float *timeslot)
{ (void)name; (void)timeslot; }

// sendto_hook is defined in bot_query_hook.o
ssize_t sendto_hook(int socket, const void *message, size_t length, int flags,
                    const struct sockaddr *dest_addr, socklen_t dest_len)
{
   (void)socket; (void)message; (void)length; (void)flags;
   (void)dest_addr; (void)dest_len;
   return (ssize_t)length;
}

// ============================================================
// Tests
// ============================================================

static int test_placeholder(void)
{
   TEST("bot_query_hook_linux.cpp placeholder");

   mock_reset();

   // Verify hook/unhook functions exist and are callable
   // Note: actually hooking sendto would modify libc, so just verify linkage
   extern bool hook_sendto_function(void);
   extern bool unhook_sendto_function(void);

   // unhook should succeed when not hooked
   ASSERT_TRUE(unhook_sendto_function() == true);

   PASS();
   return 0;
}

// ============================================================
// main
// ============================================================

int main(void)
{
   int fail = 0;

   printf("test_bot_query_hook_linux:\n");
   fail |= test_placeholder();

   printf("\n%d/%d tests passed\n", tests_passed, tests_run);
   return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
