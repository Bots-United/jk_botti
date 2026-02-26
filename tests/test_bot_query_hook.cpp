//
// JK_Botti - tests for bot_query_hook.cpp
//
// test_bot_query_hook.cpp
//
// Uses #include approach so we can test static functions like msg_get_string,
// handle_player_reply, handle_goldsrc_server_info_reply, handle_source_server_info_reply.
//

#include <stdlib.h>
#include <string.h>

#include <extdll.h>
#include <dllapi.h>
#include <h_export.h>
#include <meta_api.h>

#include "bot.h"

#include "engine_mock.h"

// bot_query_hook.h lacks include guards, so we provide PASCAL and socket types
// before bot_query_hook.cpp is #included (which includes bot_query_hook.h itself).
#include <sys/types.h>
#include <sys/socket.h>
#ifndef PASCAL
#define PASCAL
#endif

// bot_conntimes is declared extern in bot_query_hook.h, must define before #include
int bot_conntimes = 0;

// ============================================================
// Mock state for call_original_sendto
// ============================================================

static const void *last_sendto_message = NULL;
static size_t last_sendto_length = 0;
static unsigned char last_sendto_buf[4096]; // persistent copy for content checks

// Provide call_original_sendto before #including bot_query_hook.cpp
ssize_t PASCAL call_original_sendto(int socket, const void *message, size_t length,
                                    int flags, const struct sockaddr *dest_addr,
                                    socklen_t dest_len)
{
   (void)socket; (void)flags; (void)dest_addr; (void)dest_len;
   last_sendto_message = message;
   last_sendto_length = length;
   if (length <= sizeof(last_sendto_buf))
      memcpy(last_sendto_buf, message, length);
   return (ssize_t)length;
}

// ============================================================
// Mock BotReplaceConnectionTime: replace time with 999.0
// ============================================================

static int replace_conn_time_called = 0;
static char replace_conn_time_name[64] = "";

void BotReplaceConnectionTime(const char *name, float *timeslot)
{
   replace_conn_time_called++;
   safe_strcopy(replace_conn_time_name, sizeof(replace_conn_time_name), name);
   if (timeslot) *timeslot = 999.0f;
}

// Now include the source under test
#include "../bot_query_hook.cpp"

#include "test_common.h"

// ============================================================
// Helpers
// ============================================================

static void reset_test_state(void)
{
   mock_reset();
   last_sendto_message = NULL;
   last_sendto_length = 0;
   memset(last_sendto_buf, 0, sizeof(last_sendto_buf));
   replace_conn_time_called = 0;
   memset(replace_conn_time_name, 0, sizeof(replace_conn_time_name));
}

// Build a 4-byte 0xFF header + type byte
static void build_header(unsigned char *buf, char type)
{
   buf[0] = 0xFF;
   buf[1] = 0xFF;
   buf[2] = 0xFF;
   buf[3] = 0xFF;
   buf[4] = (unsigned char)type;
}

// Append a null-terminated string to buffer at offset, return new offset
static size_t append_string(unsigned char *buf, size_t offset, const char *str)
{
   size_t len = strlen(str);
   memcpy(buf + offset, str, len + 1);  // include null terminator
   return offset + len + 1;
}

// ============================================================
// msg_get_string tests
// ============================================================

static int test_msg_get_string_basic(void)
{
   TEST("msg_get_string: reads null-terminated string");
   reset_test_state();

   unsigned char data[] = { 'H', 'e', 'l', 'l', 'o', 0, 'X' };
   const unsigned char *msg = data;
   size_t len = sizeof(data);
   char name[32] = "";

   bool ok = msg_get_string(msg, len, name, sizeof(name));

   ASSERT_TRUE(ok);
   ASSERT_STR(name, "Hello");
   // msg should point past the null to 'X'
   ASSERT_INT(*msg, 'X');
   ASSERT_INT((int)len, 1);

   PASS();
   return 0;
}

static int test_msg_get_string_truncation(void)
{
   TEST("msg_get_string: truncates to nlen-1");
   reset_test_state();

   unsigned char data[] = { 'A', 'B', 'C', 'D', 'E', 'F', 0, 'X' };
   const unsigned char *msg = data;
   size_t len = sizeof(data);
   char name[4] = "";

   bool ok = msg_get_string(msg, len, name, sizeof(name));

   ASSERT_TRUE(ok);
   ASSERT_STR(name, "ABC");  // truncated to 3 chars + null

   PASS();
   return 0;
}

static int test_msg_get_string_empty(void)
{
   TEST("msg_get_string: empty string");
   reset_test_state();

   unsigned char data[] = { 0, 'X' };
   const unsigned char *msg = data;
   size_t len = sizeof(data);
   char name[32] = "old";

   bool ok = msg_get_string(msg, len, name, sizeof(name));

   ASSERT_TRUE(ok);
   ASSERT_STR(name, "");

   PASS();
   return 0;
}

static int test_msg_get_string_no_data_left(void)
{
   TEST("msg_get_string: runs out of data before null -> false");
   reset_test_state();

   unsigned char data[] = { 'A', 'B' };
   const unsigned char *msg = data;
   size_t len = sizeof(data);
   char name[32] = "";

   bool ok = msg_get_string(msg, len, name, sizeof(name));

   ASSERT_FALSE(ok);

   PASS();
   return 0;
}

// ============================================================
// sendto_hook routing tests
// ============================================================

static int test_sendto_hook_passthrough_no_globals(void)
{
   TEST("sendto_hook: passthrough when gpGlobals=NULL");
   reset_test_state();

   globalvars_t *saved_gpGlobals = gpGlobals;
   gpGlobals = NULL;
   bot_conntimes = 1;

   unsigned char buf[] = { 0xFF, 0xFF, 0xFF, 0xFF, 'D', 0x01, 0x00 };
   ssize_t ret = sendto_hook(0, buf, sizeof(buf), 0, NULL, 0);

   // Restore gpGlobals before any assertions
   gpGlobals = saved_gpGlobals;

   ASSERT_INT((int)ret, (int)sizeof(buf));
   // Should call original, not modify
   ASSERT_TRUE(last_sendto_message == buf);

   PASS();
   return 0;
}

static int test_sendto_hook_passthrough_no_bots(void)
{
   TEST("sendto_hook: passthrough when bot_conntimes=0");
   reset_test_state();
   bot_conntimes = 0;

   unsigned char buf[] = { 0xFF, 0xFF, 0xFF, 0xFF, 'D', 0x01, 0x00 };
   ssize_t ret = sendto_hook(0, buf, sizeof(buf), 0, NULL, 0);

   ASSERT_INT((int)ret, (int)sizeof(buf));
   ASSERT_TRUE(last_sendto_message == buf);

   PASS();
   return 0;
}

static int test_sendto_hook_short_packet(void)
{
   TEST("sendto_hook: short packet (<=5 bytes) passthrough");
   reset_test_state();
   bot_conntimes = 1;

   unsigned char buf[] = { 0xFF, 0xFF, 0xFF, 0xFF, 'D' };
   ssize_t ret = sendto_hook(0, buf, 5, 0, NULL, 0);

   ASSERT_INT((int)ret, 5);
   ASSERT_TRUE(last_sendto_message == buf);

   PASS();
   return 0;
}

static int test_sendto_hook_routes_D_to_player_reply(void)
{
   TEST("sendto_hook: 'D' packet routed to handle_player_reply");
   reset_test_state();
   bot_conntimes = 1;

   // Minimal player reply: header(4) + 'D' + player_count(0)
   unsigned char buf[7];
   build_header(buf, 'D');
   buf[5] = 0;  // player count = 0
   buf[6] = 0;  // padding

   ssize_t ret = sendto_hook(0, buf, sizeof(buf), 0, NULL, 0);
   ASSERT_INT((int)ret, (int)sizeof(buf));
   // last_sendto_message should be a copy (alloca buffer), not the original
   ASSERT_TRUE(last_sendto_message != buf);

   PASS();
   return 0;
}

static int test_sendto_hook_routes_m_to_goldsrc(void)
{
   TEST("sendto_hook: 'm' packet routed to goldsrc handler");
   reset_test_state();
   bot_conntimes = 1;

   // Minimal packet with 'm' type, but too short to parse fully
   unsigned char buf[7];
   build_header(buf, 'm');
   buf[5] = 0;
   buf[6] = 0;

   ssize_t ret = sendto_hook(0, buf, sizeof(buf), 0, NULL, 0);
   ASSERT_INT((int)ret, (int)sizeof(buf));
   // Should call call_original_sendto with a copy
   ASSERT_TRUE(last_sendto_message != buf);

   PASS();
   return 0;
}

static int test_sendto_hook_routes_I_to_source(void)
{
   TEST("sendto_hook: 'I' packet routed to source handler");
   reset_test_state();
   bot_conntimes = 1;

   unsigned char buf[7];
   build_header(buf, 'I');
   buf[5] = 0;
   buf[6] = 0;

   ssize_t ret = sendto_hook(0, buf, sizeof(buf), 0, NULL, 0);
   ASSERT_INT((int)ret, (int)sizeof(buf));
   ASSERT_TRUE(last_sendto_message != buf);

   PASS();
   return 0;
}

static int test_sendto_hook_unknown_type_passthrough(void)
{
   TEST("sendto_hook: unknown type byte passthrough");
   reset_test_state();
   bot_conntimes = 1;

   unsigned char buf[7];
   build_header(buf, 'Z');
   buf[5] = 0;
   buf[6] = 0;

   ssize_t ret = sendto_hook(0, buf, sizeof(buf), 0, NULL, 0);
   ASSERT_INT((int)ret, (int)sizeof(buf));
   // Unknown type -> passthrough, original buffer
   ASSERT_TRUE(last_sendto_message == buf);

   PASS();
   return 0;
}

// ============================================================
// handle_player_reply tests
// ============================================================

static int test_player_reply_with_one_player(void)
{
   TEST("handle_player_reply: calls BotReplaceConnectionTime");
   reset_test_state();
   bot_conntimes = 1;

   // Build a player reply packet:
   // header(4) + 'D' + count(1) + player_number(0) + name("Bot\0") + score(4) + time(4)
   unsigned char buf[256];
   build_header(buf, 'D');
   size_t off = 5;
   buf[off++] = 1;   // player count
   buf[off++] = 0;   // player number

   // Player name "Bot\0"
   off = append_string(buf, off, "Bot");

   // Score: 4 bytes (int32)
   int score = 5;
   memcpy(buf + off, &score, 4);
   off += 4;

   // Time: 4 bytes (float32)
   float time = 123.0f;
   memcpy(buf + off, &time, 4);
   off += 4;

   ssize_t ret = handle_player_reply(0, buf, off, 0, NULL, 0);
   ASSERT_INT((int)ret, (int)off);
   ASSERT_INT(replace_conn_time_called, 1);
   ASSERT_STR(replace_conn_time_name, "Bot");

   PASS();
   return 0;
}

static int test_player_reply_too_large(void)
{
   TEST("handle_player_reply: >4096 bytes passthrough");
   reset_test_state();

   unsigned char buf[8];
   build_header(buf, 'D');
   buf[5] = 0;

   // Fake length > 4096 -> passthrough
   ssize_t ret = handle_player_reply(0, buf, 5000, 0, NULL, 0);
   ASSERT_INT((int)ret, 5000);
   // Should use original buffer (no alloca copy)
   ASSERT_TRUE(last_sendto_message == buf);

   PASS();
   return 0;
}

// ============================================================
// handle_source_server_info_reply tests
// ============================================================

static int test_source_info_zeros_bot_count(void)
{
   TEST("handle_source_server_info: zeroes bot count byte");
   reset_test_state();
   gpGlobals->maxClients = 32;

   // Build a source server info packet:
   // header(4) + 'I' + protocol(1) + name\0 + map\0 + folder\0 + game\0
   // + steamid(2) + players(1) + maxplayers(1) + bots(1) + ...
   unsigned char buf[256];
   build_header(buf, 'I');
   size_t off = 5;

   buf[off++] = 48;  // protocol

   off = append_string(buf, off, "Test Server");
   off = append_string(buf, off, "de_dust2");
   off = append_string(buf, off, "valve");
   off = append_string(buf, off, "Half-Life");

   // Steam app ID (short = 2 bytes)
   buf[off++] = 0x46; // 70 = HL
   buf[off++] = 0x00;

   buf[off++] = 10;  // players
   buf[off++] = 32;  // max players (must match gpGlobals->maxClients)
   buf[off++] = 5;   // bots (this should be zeroed)

   // server type, env, visibility, vac (we won't get here, but add padding)
   buf[off++] = 'D';
   buf[off++] = 'L';
   buf[off++] = 0;
   buf[off++] = 1;

   ssize_t ret = handle_source_server_info_reply(0, buf, off, 0, NULL, 0);
   ASSERT_INT((int)ret, (int)off);

   // The copy should have bot count zeroed
   // Read from persistent copy buffer (alloca'd original is freed on return)
   // Compute offset to bots byte: after header+protocol+4strings+steamid+players+maxplayers
   size_t bots_offset = 5 + 1;  // header + protocol
   bots_offset += strlen("Test Server") + 1;
   bots_offset += strlen("de_dust2") + 1;
   bots_offset += strlen("valve") + 1;
   bots_offset += strlen("Half-Life") + 1;
   bots_offset += 2;  // steam id
   bots_offset += 1;  // players
   bots_offset += 1;  // max players

   ASSERT_INT(last_sendto_buf[bots_offset], 0);  // bot count zeroed

   PASS();
   return 0;
}

static int test_source_info_maxclients_mismatch(void)
{
   TEST("handle_source_server_info: maxClients mismatch -> no change");
   reset_test_state();
   gpGlobals->maxClients = 16;  // doesn't match packet's 32

   unsigned char buf[256];
   build_header(buf, 'I');
   size_t off = 5;
   buf[off++] = 48;
   off = append_string(buf, off, "Server");
   off = append_string(buf, off, "map");
   off = append_string(buf, off, "valve");
   off = append_string(buf, off, "HL");
   buf[off++] = 0x46;
   buf[off++] = 0x00;
   buf[off++] = 10;  // players
   buf[off++] = 32;  // max players (doesn't match gpGlobals->maxClients=16)
   buf[off++] = 5;   // bots

   ssize_t ret = handle_source_server_info_reply(0, buf, off, 0, NULL, 0);
   ASSERT_INT((int)ret, (int)off);

   // Bot count should NOT be zeroed (maxClients mismatch, goes to 'out')
   // Read from persistent copy buffer (alloca'd original is freed on return)
   size_t bots_offset = 5 + 1;
   bots_offset += strlen("Server") + 1;
   bots_offset += strlen("map") + 1;
   bots_offset += strlen("valve") + 1;
   bots_offset += strlen("HL") + 1;
   bots_offset += 2 + 1 + 1;  // steamid + players + maxplayers

   ASSERT_INT(last_sendto_buf[bots_offset], 5);  // unchanged

   PASS();
   return 0;
}

// ============================================================
// handle_goldsrc_server_info_reply tests
// ============================================================

static int test_goldsrc_info_zeros_bot_count(void)
{
   TEST("handle_goldsrc_server_info: zeroes bot count byte");
   reset_test_state();
   gpGlobals->maxClients = 32;

   // Build GoldSrc server info:
   // header(4) + 'm' + address\0 + name\0 + map\0 + folder\0 + game\0
   // + players(1) + maxplayers(1) + protocol(1)
   // + server_type(1) + env(1) + visibility(1) + is_mod(0)(1) + vac(1) + bots(1)
   unsigned char buf[256];
   build_header(buf, 'm');
   size_t off = 5;
   off = append_string(buf, off, "127.0.0.1:27015");
   off = append_string(buf, off, "Test Server");
   off = append_string(buf, off, "de_dust2");
   off = append_string(buf, off, "valve");
   off = append_string(buf, off, "Half-Life");
   buf[off++] = 10;   // players
   buf[off++] = 32;   // max players
   buf[off++] = 48;   // protocol
   buf[off++] = 'D';  // server type
   buf[off++] = 'L';  // environment
   buf[off++] = 0;    // visibility (public)
   buf[off++] = 0;    // is_mod (no)
   buf[off++] = 1;    // VAC
   buf[off++] = 7;    // bots (should be zeroed)

   ssize_t ret = handle_goldsrc_server_info_reply(0, buf, off, 0, NULL, 0);
   ASSERT_INT((int)ret, (int)off);

   // Verify bot count is zeroed in the copy
   // Read from persistent copy buffer (alloca'd original is freed on return)
   ASSERT_INT(last_sendto_buf[off - 1], 0);  // last byte = bots, should be 0

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

   fail |= test_msg_get_string_basic();
   fail |= test_msg_get_string_truncation();
   fail |= test_msg_get_string_empty();
   fail |= test_msg_get_string_no_data_left();
   fail |= test_sendto_hook_passthrough_no_globals();
   fail |= test_sendto_hook_passthrough_no_bots();
   fail |= test_sendto_hook_short_packet();
   fail |= test_sendto_hook_routes_D_to_player_reply();
   fail |= test_sendto_hook_routes_m_to_goldsrc();
   fail |= test_sendto_hook_routes_I_to_source();
   fail |= test_sendto_hook_unknown_type_passthrough();
   fail |= test_player_reply_with_one_player();
   fail |= test_player_reply_too_large();
   fail |= test_source_info_zeros_bot_count();
   fail |= test_source_info_maxclients_mismatch();
   fail |= test_goldsrc_info_zeros_bot_count();

   printf("\n%d/%d tests passed\n", tests_passed, tests_run);
   return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
