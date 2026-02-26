//
// JK_Botti - placeholder tests for bot_query_hook.cpp
//
// test_bot_query_hook.cpp
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

ssize_t call_original_sendto(int socket, const void *message, size_t length, int flags,
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
   TEST("bot_query_hook.cpp placeholder");

   mock_reset();

   // Verify sendto_hook function exists
   extern ssize_t sendto_hook(int, const void *, size_t, int,
                              const struct sockaddr *, socklen_t);

   // With bot_conntimes=0, sendto_hook should call through to call_original_sendto
   bot_conntimes = 0;
   unsigned char buf[] = { 0xff, 0xff, 0xff, 0xff, 'D', 0x00 };
   ssize_t ret = sendto_hook(0, buf, sizeof(buf), 0, NULL, 0);
   ASSERT_INT((int)ret, (int)sizeof(buf));

   PASS();
   return 0;
}

// ============================================================
// main
// ============================================================

int main(void)
{
   int fail = 0;

   printf("test_bot_query_hook:\n");
   fail |= test_placeholder();

   printf("\n%d/%d tests passed\n", tests_passed, tests_run);
   return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
