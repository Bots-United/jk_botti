//
// JK_Botti - tests for bot_query_hook_linux.cpp
//
// test_bot_query_hook_linux.cpp
//
// Uses #include approach to test static functions:
// construct_jmp_instruction, hook/unhook mechanics.
//

#include <stdlib.h>
#include <string.h>

// ============================================================
// Stubs for bot_query_hook.h symbols
// ============================================================

int bot_conntimes = 0;

void UTIL_ConsolePrintf(const char *fmt, ...) { (void)fmt; }

void BotReplaceConnectionTime(const char *name, float *timeslot)
{ (void)name; (void)timeslot; }

// sendto_hook is used by __replacement_sendto
#include <sys/types.h>
#include <sys/socket.h>
ssize_t sendto_hook(int socket, const void *message, size_t length, int flags,
                    const struct sockaddr *dest_addr, socklen_t dest_len)
{
   (void)socket; (void)message; (void)flags;
   (void)dest_addr; (void)dest_len;
   return (ssize_t)length;
}

// Include the source under test
#include "../bot_query_hook_linux.cpp"

#include "test_common.h"

// ============================================================
// Tests for construct_jmp_instruction
// ============================================================

static int test_construct_jmp_basic(void)
{
   TEST("construct_jmp_instruction: opcode is 0xE9");

   unsigned char buf[8];
   memset(buf, 0, sizeof(buf));

   // Place a dummy place and target at known addresses
   // place = 0x1000, target = 0x2000
   // relative offset = target - (place + 5) = 0x2000 - 0x1005 = 0x0FFB
   void *place = (void *)0x1000;
   void *target = (void *)0x2000;

   construct_jmp_instruction(buf, place, target);

   ASSERT_INT(buf[0], 0xE9);

   // Check relative offset
   unsigned long offset = *(unsigned long *)(buf + 1);
   unsigned long expected = (unsigned long)target - ((unsigned long)place + 5);
   ASSERT_TRUE(offset == expected);

   PASS();
   return 0;
}

static int test_construct_jmp_backward(void)
{
   TEST("construct_jmp_instruction: backward jump offset");

   unsigned char buf[8];
   memset(buf, 0, sizeof(buf));

   void *place = (void *)0x2000;
   void *target = (void *)0x1000;

   construct_jmp_instruction(buf, place, target);

   ASSERT_INT(buf[0], 0xE9);

   unsigned long offset = *(unsigned long *)(buf + 1);
   unsigned long expected = (unsigned long)target - ((unsigned long)place + 5);
   ASSERT_TRUE(offset == expected);

   PASS();
   return 0;
}

// ============================================================
// Tests for hook/unhook state machine
// ============================================================

static int test_unhook_when_not_hooked(void)
{
   TEST("unhook_sendto_function: not hooked -> true");

   is_sendto_hook_setup = false;
   ASSERT_TRUE(unhook_sendto_function() == true);
   ASSERT_TRUE(is_sendto_hook_setup == false);

   PASS();
   return 0;
}

static int test_hook_sendto(void)
{
   TEST("hook_sendto_function: hooks sendto and writes JMP");

   is_sendto_hook_setup = false;
   sendto_original = NULL;
   memset(sendto_new_bytes, 0, sizeof(sendto_new_bytes));
   memset(sendto_old_bytes, 0, sizeof(sendto_old_bytes));

   bool ok = hook_sendto_function();
   ASSERT_TRUE(ok);
   ASSERT_TRUE(is_sendto_hook_setup);

   // sendto_original should be non-null (resolved &sendto)
   ASSERT_TRUE(sendto_original != NULL);

   // The first byte of sendto should now be 0xE9 (JMP)
   ASSERT_INT(((unsigned char *)sendto_original)[0], 0xE9);

   // The old bytes should have been saved
   // We can verify by unhooking and checking restoration
   ASSERT_TRUE(unhook_sendto_function());
   ASSERT_TRUE(is_sendto_hook_setup == false);

   // After unhook, sendto should have its original first bytes restored
   ASSERT_INT(memcmp((void *)sendto_original, sendto_old_bytes, BYTES_SIZE), 0);

   PASS();
   return 0;
}

static int test_hook_already_hooked(void)
{
   TEST("hook_sendto_function: already hooked -> true immediately");

   is_sendto_hook_setup = false;
   ASSERT_TRUE(hook_sendto_function());
   ASSERT_TRUE(is_sendto_hook_setup);

   // Second call should return true without re-hooking
   ASSERT_TRUE(hook_sendto_function());

   // Cleanup
   unhook_sendto_function();

   PASS();
   return 0;
}

static int test_call_original_sendto_via_sendmsg(void)
{
   TEST("call_original_sendto: uses sendmsg internally");

   // call_original_sendto uses sendmsg to bypass the hook.
   // We can't easily test the actual system call, but we can verify
   // it doesn't crash and returns something reasonable with an invalid socket.
   // Use socket -1 which should fail with -1.
   unsigned char buf[] = "test";
   ssize_t ret = call_original_sendto(-1, buf, sizeof(buf), 0, NULL, 0);

   // With invalid socket, sendmsg should return -1
   ASSERT_INT((int)ret, -1);

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

   fail |= test_construct_jmp_basic();
   fail |= test_construct_jmp_backward();
   fail |= test_unhook_when_not_hooked();
   fail |= test_hook_sendto();
   fail |= test_hook_already_hooked();
   fail |= test_call_original_sendto_via_sendmsg();

   printf("\n%d/%d tests passed\n", tests_passed, tests_run);
   return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
