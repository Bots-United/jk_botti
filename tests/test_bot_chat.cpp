//
// JK_Botti - unit tests for bot_chat.cpp
//
// test_bot_chat.cpp
//
// Uses the #include-the-.cpp approach to access static functions directly.
//

#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>

// Define the 4 extern globals that bot_chat.cpp needs
int bot_chat_tag_percent;
int bot_chat_drop_percent;
int bot_chat_swap_percent;
int bot_chat_lower_percent;

// Include the source file under test (brings in all its headers + static functions)
#include "../bot_chat.cpp"

// Test infrastructure
#include "engine_mock.h"
#include "test_common.h"

// ============================================================
// Test helpers
// ============================================================

static void reset_chat_state(void)
{
   mock_reset();

   bot_chat_count = 0;
   bot_taunt_count = 0;
   bot_whine_count = 0;
   bot_endgame_count = 0;

   for (int i = 0; i < 5; i++)
   {
      recent_bot_chat[i] = -1;
      recent_bot_taunt[i] = -1;
      recent_bot_whine[i] = -1;
      recent_bot_endgame[i] = -1;
   }

   player_count = 0;
   memset(player_names, 0, sizeof(player_names));

   bot_chat_tag_percent = 0;
   bot_chat_drop_percent = 0;
   bot_chat_swap_percent = 0;
   bot_chat_lower_percent = 0;

   fast_random_seed(42);
}

static void mock_set_netname(edict_t *e, const char *name)
{
   e->v.netname = (string_t)(long)name;
}

static void setup_bot_for_chat_test(bot_t &pBot, edict_t *pEdict)
{
   memset(&pBot, 0, sizeof(pBot));
   pBot.pEdict = pEdict;
   pBot.is_used = TRUE;
   mock_set_netname(pEdict, "TestBot");
   pBot.taunt_percent = 100;
   pBot.whine_percent = 100;
   pBot.chat_percent = 100;
   pBot.endgame_percent = 100;
}

static void setup_mock_player(int edict_index, const char *name)
{
   mock_edicts[edict_index].free = 0;
   mock_edicts[edict_index].v.flags = FL_CLIENT;
   mock_set_netname(&mock_edicts[edict_index], name);
}

// ============================================================
// Temp file helpers for LoadBotChat tests
// ============================================================

// Suppress warn_unused_result for chdir/getcwd (test-only, not safety-critical)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"

static char test_tmpdir[256];
static char saved_cwd[256];
static int temp_initialized = 0;

static void init_temp_dir(void)
{
   if (!temp_initialized)
   {
      getcwd(saved_cwd, sizeof(saved_cwd));
      snprintf(test_tmpdir, sizeof(test_tmpdir), "/tmp/jk_botti_test_XXXXXX");
      if (!mkdtemp(test_tmpdir))
      {
         perror("mkdtemp");
         exit(1);
      }

      char path[512];
      snprintf(path, sizeof(path), "%s/valve", test_tmpdir);
      mkdir(path, 0755);
      snprintf(path, sizeof(path), "%s/valve/addons", test_tmpdir);
      mkdir(path, 0755);
      snprintf(path, sizeof(path), "%s/valve/addons/jk_botti", test_tmpdir);
      mkdir(path, 0755);

      temp_initialized = 1;
   }
}

static int create_chat_file(const char *content)
{
   init_temp_dir();

   char path[512];
   snprintf(path, sizeof(path), "%s/valve/addons/jk_botti/jk_botti_chat.txt",
            test_tmpdir);

   FILE *fp = fopen(path, "w");
   if (!fp) return -1;
   fputs(content, fp);
   fclose(fp);

   return chdir(test_tmpdir);
}

static void remove_chat_file(void)
{
   char path[512];
   snprintf(path, sizeof(path), "%s/valve/addons/jk_botti/jk_botti_chat.txt",
            test_tmpdir);
   unlink(path);
   chdir(saved_cwd);
}

static void cleanup_temp_dir(void)
{
   if (temp_initialized)
   {
      chdir(saved_cwd);

      char path[512];
      snprintf(path, sizeof(path), "%s/valve/addons/jk_botti/jk_botti_chat.txt",
               test_tmpdir);
      unlink(path);
      snprintf(path, sizeof(path), "%s/valve/addons/jk_botti", test_tmpdir);
      rmdir(path);
      snprintf(path, sizeof(path), "%s/valve/addons", test_tmpdir);
      rmdir(path);
      snprintf(path, sizeof(path), "%s/valve", test_tmpdir);
      rmdir(path);
      rmdir(test_tmpdir);

      temp_initialized = 0;
   }
}

#pragma GCC diagnostic pop

// ============================================================
// BotTrimBlanks tests
// ============================================================

static int test_BotTrimBlanks(void)
{
   char out[80];

   printf("BotTrimBlanks:\n");

   TEST("no blanks");
   BotTrimBlanks("hello", out, sizeof(out));
   ASSERT_STR(out, "hello");
   PASS();

   TEST("leading blanks");
   BotTrimBlanks("   hello", out, sizeof(out));
   ASSERT_STR(out, "hello");
   PASS();

   TEST("trailing blanks");
   BotTrimBlanks("hello   ", out, sizeof(out));
   ASSERT_STR(out, "hello");
   PASS();

   TEST("leading and trailing blanks");
   BotTrimBlanks("  hello  ", out, sizeof(out));
   ASSERT_STR(out, "hello");
   PASS();

   TEST("all blanks yields empty string");
   BotTrimBlanks("     ", out, sizeof(out));
   ASSERT_STR(out, "");
   PASS();

   TEST("empty string");
   BotTrimBlanks("", out, sizeof(out));
   ASSERT_STR(out, "");
   PASS();

   TEST("single non-blank char");
   BotTrimBlanks("x", out, sizeof(out));
   ASSERT_STR(out, "x");
   PASS();

   TEST("middle spaces preserved");
   BotTrimBlanks("  hello world  ", out, sizeof(out));
   ASSERT_STR(out, "hello world");
   PASS();

   return 0;
}

// ============================================================
// BotChatTrimTag tests
// ============================================================

static int test_BotChatTrimTag(void)
{
   char out[80];
   int result;

   printf("BotChatTrimTag:\n");

   TEST("no tag returns 0, name unchanged");
   result = BotChatTrimTag("Player", out, sizeof(out));
   ASSERT_INT(result, 0);
   ASSERT_STR(out, "Player");
   PASS();

   // tag pair 14: "[" / "]"
   TEST("[CLAN]Name stripped, returns 1");
   result = BotChatTrimTag("[CLAN]Player", out, sizeof(out));
   ASSERT_INT(result, 1);
   ASSERT_STR(out, "Player");
   PASS();

   // tag pair 2: "-=" / "=-"
   TEST("-=TAG=-Name stripped, returns 1");
   result = BotChatTrimTag("-=TAG=-Player", out, sizeof(out));
   ASSERT_INT(result, 1);
   ASSERT_STR(out, "Player");
   PASS();

   // tag pair 0: "[" / "]*"
   TEST("[TAG]*Name stripped, returns 1");
   result = BotChatTrimTag("[TAG]*Name", out, sizeof(out));
   ASSERT_INT(result, 1);
   ASSERT_STR(out, "Name");
   PASS();

   // Name is just a tag -> restores original, returns 0
   TEST("name is just a tag -> returns 0, original preserved");
   result = BotChatTrimTag("[TAG]*", out, sizeof(out));
   ASSERT_INT(result, 0);
   ASSERT_STR(out, "[TAG]*");
   PASS();

   TEST("empty input returns 0, empty output");
   result = BotChatTrimTag("", out, sizeof(out));
   ASSERT_INT(result, 0);
   ASSERT_STR(out, "");
   PASS();

   // Leading tag with trailing name content
   TEST("-{CLAN}-Player stripped");
   result = BotChatTrimTag("-{CLAN}-Player", out, sizeof(out));
   ASSERT_INT(result, 1);
   ASSERT_STR(out, "Player");
   PASS();

   return 0;
}

// ============================================================
// BotDropCharacter tests
// ============================================================

static int test_BotDropCharacter(void)
{
   char out[80];

   printf("BotDropCharacter:\n");

   TEST("short string (1 char) -> no change");
   fast_random_seed(42);
   BotDropCharacter("x", out, sizeof(out));
   ASSERT_STR(out, "x");
   PASS();

   TEST("empty string -> no change");
   BotDropCharacter("", out, sizeof(out));
   ASSERT_STR(out, "");
   PASS();

   TEST("normal drop reduces length by 1");
   fast_random_seed(42);
   BotDropCharacter("hello world", out, sizeof(out));
   ASSERT_INT((int)strlen(out), 10);
   PASS();

   TEST("drop skips format char after %%");
   fast_random_seed(42);
   // String where all non-first positions have % before them is extreme,
   // just verify %n is not broken
   BotDropCharacter("say %n ok", out, sizeof(out));
   // %n should remain intact (n follows %, so it's skipped)
   // Output should still contain "%n"
   ASSERT_INT((int)strlen(out), 8);
   PASS();

   TEST("2-char string drops one char");
   fast_random_seed(42);
   BotDropCharacter("ab", out, sizeof(out));
   ASSERT_INT((int)strlen(out), 1);
   PASS();

   TEST("retry loop with %%-heavy string (no valid drop)");
   // All positions 1-20 are either '%' or preceded by '%', forcing
   // the retry loop to exhaust (20 iterations), no drop performed
   fast_random_seed(123);
   BotDropCharacter("a%b%c%d%e%f%g%h%i%j%k", out, sizeof(out));
   ASSERT_INT((int)strlen(out), 21);  // no drop: retry exhausted
   PASS();

   return 0;
}

// ============================================================
// BotSwapCharacter tests
// ============================================================

static int test_BotSwapCharacter(void)
{
   char out[80];

   printf("BotSwapCharacter:\n");

   TEST("short string (2 chars) -> no change");
   fast_random_seed(42);
   BotSwapCharacter("ab", out, sizeof(out));
   ASSERT_STR(out, "ab");
   PASS();

   TEST("empty string -> no change");
   BotSwapCharacter("", out, sizeof(out));
   ASSERT_STR(out, "");
   PASS();

   TEST("normal swap preserves length");
   fast_random_seed(42);
   BotSwapCharacter("hello world", out, sizeof(out));
   ASSERT_INT((int)strlen(out), 11);
   PASS();

   TEST("swap changes content for long enough string");
   fast_random_seed(42);
   BotSwapCharacter("abcdef", out, sizeof(out));
   ASSERT_INT((int)strlen(out), 6);
   // With deterministic seed, output should differ from input
   ASSERT_TRUE(strcmp(out, "abcdef") != 0);
   PASS();

   TEST("1-char string -> no change");
   BotSwapCharacter("x", out, sizeof(out));
   ASSERT_STR(out, "x");
   PASS();

   TEST("all non-alpha middle -> no swap");
   // All middle chars are digits, len < 20, retry loop exhausts at count==len
   // Bug: only checks count < 20, should also check count < len
   fast_random_seed(42);
   BotSwapCharacter("a123456789b", out, sizeof(out));
   ASSERT_STR(out, "a123456789b");
   PASS();

   return 0;
}

// ============================================================
// BotChatName tests
// ============================================================

static int test_BotChatName(void)
{
   char out[80];

   printf("BotChatName:\n");

   TEST("no modifications (both percents 0)");
   reset_chat_state();
   BotChatName("PlayerOne", out, sizeof(out));
   ASSERT_STR(out, "PlayerOne");
   PASS();

   TEST("[lvl0] stripped");
   reset_chat_state();
   BotChatName("[lvl0]Player", out, sizeof(out));
   ASSERT_STR(out, "Player");
   PASS();

   TEST("[lvl5] stripped");
   reset_chat_state();
   BotChatName("[lvl5]Player", out, sizeof(out));
   ASSERT_STR(out, "Player");
   PASS();

   TEST("[lvl6] NOT stripped (only 0-5)");
   reset_chat_state();
   BotChatName("[lvl6]Player", out, sizeof(out));
   ASSERT_STR(out, "[lvl6]Player");
   PASS();

   TEST("tag removal with tag_percent=100");
   reset_chat_state();
   bot_chat_tag_percent = 100;
   BotChatName("[CLAN]Player", out, sizeof(out));
   ASSERT_STR(out, "Player");
   PASS();

   TEST("tag NOT removed with tag_percent=0");
   reset_chat_state();
   bot_chat_tag_percent = 0;
   BotChatName("[CLAN]Player", out, sizeof(out));
   ASSERT_STR(out, "[CLAN]Player");
   PASS();

   TEST("lowercase with lower_percent=100");
   reset_chat_state();
   bot_chat_lower_percent = 100;
   BotChatName("PlayerOne", out, sizeof(out));
   ASSERT_STR(out, "playerone");
   PASS();

   TEST("combined: [lvl3] strip + tag strip + lowercase");
   reset_chat_state();
   bot_chat_tag_percent = 100;
   bot_chat_lower_percent = 100;
   BotChatName("[lvl3][CLAN]Player", out, sizeof(out));
   ASSERT_STR(out, "player");
   PASS();

   return 0;
}

// ============================================================
// BotChatText tests
// ============================================================

static int test_BotChatText(void)
{
   char out[81];

   printf("BotChatText:\n");

   TEST("no modifications (all percents 0)");
   reset_chat_state();
   BotChatText("Hello World", out, sizeof(out));
   ASSERT_STR(out, "Hello World");
   PASS();

   TEST("lowercase only");
   reset_chat_state();
   bot_chat_lower_percent = 100;
   BotChatText("Hello World", out, sizeof(out));
   ASSERT_STR(out, "hello world");
   PASS();

   TEST("drop enabled reduces length");
   reset_chat_state();
   bot_chat_drop_percent = 100;
   BotChatText("Hello World", out, sizeof(out));
   ASSERT_TRUE(strlen(out) < strlen("Hello World"));
   PASS();

   TEST("empty input stays empty");
   reset_chat_state();
   bot_chat_lower_percent = 100;
   bot_chat_drop_percent = 100;
   bot_chat_swap_percent = 100;
   BotChatText("", out, sizeof(out));
   ASSERT_STR(out, "");
   PASS();

   return 0;
}

// ============================================================
// BotChatGetPlayers tests
// ============================================================

static int test_BotChatGetPlayers(void)
{
   printf("BotChatGetPlayers:\n");

   TEST("no valid players -> count=0");
   reset_chat_state();
   BotChatGetPlayers();
   ASSERT_INT(player_count, 0);
   PASS();

   TEST("one player");
   reset_chat_state();
   setup_mock_player(1, "Alice");
   BotChatGetPlayers();
   ASSERT_INT(player_count, 1);
   ASSERT_STR(player_names[0], "Alice");
   PASS();

   TEST("multiple players");
   reset_chat_state();
   setup_mock_player(1, "Alice");
   setup_mock_player(2, "Bob");
   setup_mock_player(3, "Charlie");
   BotChatGetPlayers();
   ASSERT_INT(player_count, 3);
   ASSERT_STR(player_names[0], "Alice");
   ASSERT_STR(player_names[1], "Bob");
   ASSERT_STR(player_names[2], "Charlie");
   PASS();

   TEST("skip free edict");
   reset_chat_state();
   setup_mock_player(1, "Alice");
   setup_mock_player(2, "Bob");
   mock_edicts[2].free = 1;
   BotChatGetPlayers();
   ASSERT_INT(player_count, 1);
   ASSERT_STR(player_names[0], "Alice");
   PASS();

   TEST("skip FL_PROXY");
   reset_chat_state();
   setup_mock_player(1, "Alice");
   setup_mock_player(2, "HLTVProxy");
   mock_edicts[2].v.flags |= FL_PROXY;
   BotChatGetPlayers();
   ASSERT_INT(player_count, 1);
   ASSERT_STR(player_names[0], "Alice");
   PASS();

   TEST("skip empty netname (string_t == 0)");
   reset_chat_state();
   setup_mock_player(1, "Alice");
   // edict 2 has flags but no netname
   mock_edicts[2].free = 0;
   mock_edicts[2].v.flags = FL_CLIENT;
   // netname stays 0 from mock_reset
   BotChatGetPlayers();
   ASSERT_INT(player_count, 1);
   PASS();

   TEST("skip zero-length netname");
   reset_chat_state();
   setup_mock_player(1, "Alice");
   setup_mock_player(2, "");
   BotChatGetPlayers();
   ASSERT_INT(player_count, 1);
   ASSERT_STR(player_names[0], "Alice");
   PASS();

   return 0;
}

// ============================================================
// BotChatFillInName tests
// ============================================================

static int test_BotChatFillInName(void)
{
   char out[256];

   printf("BotChatFillInName:\n");

   TEST("no substitutions");
   reset_chat_state();
   BotChatFillInName(out, sizeof(out), "hello world", "Victim", "Bot");
   ASSERT_STR(out, "hello world");
   PASS();

   TEST("%%n replaced with chat_name");
   reset_chat_state();
   BotChatFillInName(out, sizeof(out), "got you %n", "Victim", "Bot");
   ASSERT_STR(out, "got you Victim");
   PASS();

   TEST("multiple %%n");
   reset_chat_state();
   BotChatFillInName(out, sizeof(out), "%n vs %n", "Victim", "Bot");
   ASSERT_STR(out, "Victim vs Victim");
   PASS();

   TEST("%%r with players available");
   reset_chat_state();
   setup_mock_player(1, "RandomGuy");
   // With only 1 player, %r picks that player (even if same as chat_name/bot_name,
   // the retry loop will exhaust and still use it)
   BotChatFillInName(out, sizeof(out), "ask %r", "Victim", "Bot");
   // The player name goes through BotChatName (with tag_percent=0, lower_percent=0 -> unchanged)
   ASSERT_STR(out, "ask RandomGuy");
   PASS();

   TEST("%%r with player_count==0 falls back to chat_name");
   reset_chat_state();
   // No players set up
   BotChatFillInName(out, sizeof(out), "ask %r", "Victim", "Bot");
   ASSERT_STR(out, "ask Victim");
   PASS();

   TEST("unknown %%x passed through literally");
   reset_chat_state();
   BotChatFillInName(out, sizeof(out), "hello %x there", "Victim", "Bot");
   ASSERT_STR(out, "hello %x there");
   PASS();

   TEST("dangling %% at end");
   reset_chat_state();
   BotChatFillInName(out, sizeof(out), "hello %", "Victim", "Bot");
   ASSERT_STR(out, "hello %");
   PASS();

   TEST("empty input");
   reset_chat_state();
   BotChatFillInName(out, sizeof(out), "", "Victim", "Bot");
   ASSERT_STR(out, "");
   PASS();

   TEST("truncation at buffer limit");
   reset_chat_state();
   char small[10];
   BotChatFillInName(small, sizeof(small), "got you %n loser", "VictimName", "Bot");
   // Buffer is 10 bytes. "got you " = 8, then copying "VictimName"...
   // o starts at 0: g(0) o(1) t(2) (3) y(4) o(5) u(6) (7) -> then %n
   // Copying "VictimName" while o < 10: V(8) i(9) -> o=10, stops
   // o == sizeof_msg -> bot_say_msg[9] = 0
   ASSERT_INT((int)strlen(small), 9);
   PASS();

   TEST("%%r retry when only player matches chat_name");
   reset_chat_state();
   setup_mock_player(1, "Victim");
   // Only one player "Victim" which matches chat_name; retry loop exhausts, uses anyway
   BotChatFillInName(out, sizeof(out), "ask %r", "Victim", "Bot");
   ASSERT_TRUE(strstr(out, "Victim") != NULL);
   PASS();

   TEST("%%r retry when only player matches bot_name");
   reset_chat_state();
   setup_mock_player(1, "TestBot");
   // Only one player "TestBot" which matches bot_name; retry loop exhausts
   BotChatFillInName(out, sizeof(out), "ask %r", "Other", "TestBot");
   ASSERT_TRUE(strstr(out, "TestBot") != NULL);
   PASS();

   return 0;
}

// ============================================================
// LoadBotChat tests
// ============================================================

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"

static int test_LoadBotChat(void)
{
   printf("LoadBotChat:\n");

   TEST("file not found -> counts stay 0");
   reset_chat_state();
   init_temp_dir();
   chdir(test_tmpdir);
   {
      // Ensure no leftover file
      char path[512];
      snprintf(path, sizeof(path), "%s/valve/addons/jk_botti/jk_botti_chat.txt",
               test_tmpdir);
      unlink(path);
   }
   LoadBotChat();
   ASSERT_INT(bot_chat_count, 0);
   ASSERT_INT(bot_taunt_count, 0);
   ASSERT_INT(bot_whine_count, 0);
   ASSERT_INT(bot_endgame_count, 0);
   chdir(saved_cwd);
   PASS();

   TEST("all 4 sections loaded");
   reset_chat_state();
   create_chat_file(
      "[bot_chat]\n"
      "hello there\n"
      "how are you\n"
      "[bot_taunt]\n"
      "got you %n\n"
      "[bot_whine]\n"
      "why me\n"
      "unfair\n"
      "lucky shot\n"
      "[bot_endgame]\n"
      "gg\n"
   );
   LoadBotChat();
   ASSERT_INT(bot_chat_count, 2);
   ASSERT_INT(bot_taunt_count, 1);
   ASSERT_INT(bot_whine_count, 3);
   ASSERT_INT(bot_endgame_count, 1);
   ASSERT_STR(bot_chat[0].text, "hello there");
   ASSERT_STR(bot_chat[1].text, "how are you");
   ASSERT_STR(bot_taunt[0].text, "got you %n");
   ASSERT_STR(bot_whine[0].text, "why me");
   ASSERT_STR(bot_endgame[0].text, "gg");
   remove_chat_file();
   PASS();

   TEST("! prefix sets can_modify=FALSE and strips !");
   reset_chat_state();
   create_chat_file(
      "[bot_chat]\n"
      "!dont modify this\n"
      "modify this\n"
   );
   LoadBotChat();
   ASSERT_INT(bot_chat_count, 2);
   ASSERT_STR(bot_chat[0].text, "dont modify this");
   ASSERT_INT(bot_chat[0].can_modify, FALSE);
   ASSERT_STR(bot_chat[1].text, "modify this");
   ASSERT_INT(bot_chat[1].can_modify, TRUE);
   remove_chat_file();
   PASS();

   TEST("lone ! is skipped");
   reset_chat_state();
   create_chat_file(
      "[bot_chat]\n"
      "!\n"
      "real message\n"
   );
   LoadBotChat();
   ASSERT_INT(bot_chat_count, 1);
   ASSERT_STR(bot_chat[0].text, "real message");
   remove_chat_file();
   PASS();

   TEST("empty lines are skipped");
   reset_chat_state();
   create_chat_file(
      "[bot_chat]\n"
      "\n"
      "hello\n"
      "\n"
   );
   LoadBotChat();
   ASSERT_INT(bot_chat_count, 1);
   ASSERT_STR(bot_chat[0].text, "hello");
   remove_chat_file();
   PASS();

   TEST("lines before first section are ignored");
   reset_chat_state();
   create_chat_file(
      "this is ignored\n"
      "[bot_chat]\n"
      "hello\n"
   );
   LoadBotChat();
   ASSERT_INT(bot_chat_count, 1);
   ASSERT_STR(bot_chat[0].text, "hello");
   remove_chat_file();
   PASS();

   TEST("recent arrays initialized to -1");
   reset_chat_state();
   create_chat_file("[bot_chat]\nhello\n");
   LoadBotChat();
   for (int i = 0; i < 5; i++)
   {
      ASSERT_INT(recent_bot_chat[i], -1);
      ASSERT_INT(recent_bot_taunt[i], -1);
      ASSERT_INT(recent_bot_whine[i], -1);
      ASSERT_INT(recent_bot_endgame[i], -1);
   }
   remove_chat_file();
   PASS();

   TEST("MAX_BOT_CHAT limit respected");
   reset_chat_state();
   {
      // Build a file with MAX_BOT_CHAT + 2 lines in [bot_chat]
      char big_content[16384];
      int pos = 0;
      pos += snprintf(big_content + pos, sizeof(big_content) - pos, "[bot_chat]\n");
      for (int i = 0; i < MAX_BOT_CHAT + 2; i++)
         pos += snprintf(big_content + pos, sizeof(big_content) - pos, "msg%d\n", i);
      create_chat_file(big_content);
   }
   LoadBotChat();
   ASSERT_INT(bot_chat_count, MAX_BOT_CHAT);
   remove_chat_file();
   PASS();

   TEST("! prefix in taunt/whine/endgame sections");
   reset_chat_state();
   create_chat_file(
      "[bot_taunt]\n"
      "!no modify taunt\n"
      "modify taunt\n"
      "[bot_whine]\n"
      "!no modify whine\n"
      "modify whine\n"
      "[bot_endgame]\n"
      "!no modify endgame\n"
      "modify endgame\n"
   );
   LoadBotChat();
   ASSERT_INT(bot_taunt_count, 2);
   ASSERT_STR(bot_taunt[0].text, "no modify taunt");
   ASSERT_INT(bot_taunt[0].can_modify, FALSE);
   ASSERT_STR(bot_taunt[1].text, "modify taunt");
   ASSERT_INT(bot_taunt[1].can_modify, TRUE);
   ASSERT_INT(bot_whine_count, 2);
   ASSERT_STR(bot_whine[0].text, "no modify whine");
   ASSERT_INT(bot_whine[0].can_modify, FALSE);
   ASSERT_STR(bot_whine[1].text, "modify whine");
   ASSERT_INT(bot_whine[1].can_modify, TRUE);
   ASSERT_INT(bot_endgame_count, 2);
   ASSERT_STR(bot_endgame[0].text, "no modify endgame");
   ASSERT_INT(bot_endgame[0].can_modify, FALSE);
   ASSERT_STR(bot_endgame[1].text, "modify endgame");
   ASSERT_INT(bot_endgame[1].can_modify, TRUE);
   remove_chat_file();
   PASS();

   return 0;
}

#pragma GCC diagnostic pop

// ============================================================
// BotChatTaunt tests
// ============================================================

static int test_BotChatTaunt(void)
{
   bot_t bot;
   edict_t *bot_edict;
   edict_t *victim;

   printf("BotChatTaunt:\n");

   TEST("skip: already chatting");
   reset_chat_state();
   bot_edict = &mock_edicts[1];
   setup_bot_for_chat_test(bot, bot_edict);
   bot.b_bot_say = TRUE;
   bot.f_bot_say = gpGlobals->time + 5.0;
   bot_taunt_count = 1;
   safe_strcopy(bot_taunt[0].text, sizeof(bot_taunt[0].text), "test");
   bot_taunt[0].can_modify = FALSE;
   victim = &mock_edicts[2];
   mock_set_netname(victim, "Victim");
   BotChatTaunt(bot, victim);
   // bot_say_msg should be empty (unchanged from setup)
   ASSERT_STR(bot.bot_say_msg, "");
   PASS();

   TEST("skip: no taunt messages");
   reset_chat_state();
   bot_edict = &mock_edicts[1];
   setup_bot_for_chat_test(bot, bot_edict);
   victim = &mock_edicts[2];
   mock_set_netname(victim, "Victim");
   BotChatTaunt(bot, victim);
   ASSERT_INT(bot.b_bot_say, FALSE);
   PASS();

   TEST("skip: percent=0");
   reset_chat_state();
   bot_edict = &mock_edicts[1];
   setup_bot_for_chat_test(bot, bot_edict);
   bot.taunt_percent = 0;
   bot_taunt_count = 1;
   safe_strcopy(bot_taunt[0].text, sizeof(bot_taunt[0].text), "test");
   bot_taunt[0].can_modify = FALSE;
   victim = &mock_edicts[2];
   mock_set_netname(victim, "Victim");
   BotChatTaunt(bot, victim);
   ASSERT_INT(bot.b_bot_say, FALSE);
   PASS();

   TEST("success: sets b_bot_say, f_bot_say, bot_say_msg");
   reset_chat_state();
   bot_edict = &mock_edicts[1];
   setup_bot_for_chat_test(bot, bot_edict);
   victim = &mock_edicts[2];
   mock_set_netname(victim, "Victim");
   bot_taunt_count = 1;
   safe_strcopy(bot_taunt[0].text, sizeof(bot_taunt[0].text), "haha loser");
   bot_taunt[0].can_modify = FALSE;
   BotChatTaunt(bot, victim);
   ASSERT_INT(bot.b_bot_say, TRUE);
   ASSERT_TRUE(bot.f_bot_say > gpGlobals->time);
   ASSERT_STR(bot.bot_say_msg, "haha loser");
   PASS();

   TEST("success with %%n substitution");
   reset_chat_state();
   bot_edict = &mock_edicts[1];
   setup_bot_for_chat_test(bot, bot_edict);
   victim = &mock_edicts[2];
   mock_set_netname(victim, "Victim");
   bot_taunt_count = 1;
   safe_strcopy(bot_taunt[0].text, sizeof(bot_taunt[0].text), "got you %n");
   bot_taunt[0].can_modify = FALSE;
   BotChatTaunt(bot, victim);
   ASSERT_STR(bot.bot_say_msg, "got you Victim");
   PASS();

   TEST("victim netname NULL -> uses 'NULL'");
   reset_chat_state();
   bot_edict = &mock_edicts[1];
   setup_bot_for_chat_test(bot, bot_edict);
   victim = &mock_edicts[2];
   // Don't set netname -> stays 0
   bot_taunt_count = 1;
   safe_strcopy(bot_taunt[0].text, sizeof(bot_taunt[0].text), "bye %n");
   bot_taunt[0].can_modify = FALSE;
   BotChatTaunt(bot, victim);
   ASSERT_STR(bot.bot_say_msg, "bye NULL");
   PASS();

   TEST("can_modify=TRUE with lowercase");
   reset_chat_state();
   bot_chat_lower_percent = 100;
   bot_edict = &mock_edicts[1];
   setup_bot_for_chat_test(bot, bot_edict);
   victim = &mock_edicts[2];
   mock_set_netname(victim, "Victim");
   bot_taunt_count = 1;
   safe_strcopy(bot_taunt[0].text, sizeof(bot_taunt[0].text), "GOT YOU");
   bot_taunt[0].can_modify = TRUE;
   BotChatTaunt(bot, victim);
   ASSERT_STR(bot.bot_say_msg, "got you");
   PASS();

   TEST("recent message deduplication loop");
   reset_chat_state();
   bot_edict = &mock_edicts[1];
   victim = &mock_edicts[2];
   mock_set_netname(victim, "Victim");
   // Load 6 taunt messages, all can_modify=FALSE
   bot_taunt_count = 6;
   for (int i = 0; i < 6; i++)
   {
      char msg[16];
      snprintf(msg, sizeof(msg), "taunt%d", i);
      safe_strcopy(bot_taunt[i].text, sizeof(bot_taunt[i].text), msg);
      bot_taunt[i].can_modify = FALSE;
   }
   // Call BotChatTaunt 7 times to fill recent array (5 slots) and trigger dedup
   for (int i = 0; i < 7; i++)
   {
      fast_random_seed(i * 31 + 7);
      setup_bot_for_chat_test(bot, bot_edict);
      BotChatTaunt(bot, victim);
      ASSERT_INT(bot.b_bot_say, TRUE);
   }
   PASS();

   return 0;
}

// ============================================================
// BotChatWhine tests
// ============================================================

static int test_BotChatWhine(void)
{
   bot_t bot;
   edict_t *bot_edict;
   edict_t *killer;

   printf("BotChatWhine:\n");

   TEST("skip: already chatting");
   reset_chat_state();
   bot_edict = &mock_edicts[1];
   setup_bot_for_chat_test(bot, bot_edict);
   bot.b_bot_say = TRUE;
   bot.f_bot_say = gpGlobals->time + 5.0;
   killer = &mock_edicts[3];
   mock_set_netname(killer, "Killer");
   bot.killer_edict = killer;
   bot.f_bot_spawn_time = 0;
   bot_whine_count = 1;
   safe_strcopy(bot_whine[0].text, sizeof(bot_whine[0].text), "test");
   bot_whine[0].can_modify = FALSE;
   BotChatWhine(bot);
   ASSERT_STR(bot.bot_say_msg, "");
   PASS();

   TEST("skip: no killer");
   reset_chat_state();
   bot_edict = &mock_edicts[1];
   setup_bot_for_chat_test(bot, bot_edict);
   // killer_edict is NULL from setup
   bot_whine_count = 1;
   safe_strcopy(bot_whine[0].text, sizeof(bot_whine[0].text), "test");
   bot_whine[0].can_modify = FALSE;
   BotChatWhine(bot);
   ASSERT_INT(bot.b_bot_say, FALSE);
   PASS();

   TEST("skip: no whine messages");
   reset_chat_state();
   bot_edict = &mock_edicts[1];
   setup_bot_for_chat_test(bot, bot_edict);
   killer = &mock_edicts[3];
   bot.killer_edict = killer;
   bot.f_bot_spawn_time = 0;
   BotChatWhine(bot);
   ASSERT_INT(bot.b_bot_say, FALSE);
   PASS();

   TEST("skip: too recently spawned (< 15s)");
   reset_chat_state();
   bot_edict = &mock_edicts[1];
   setup_bot_for_chat_test(bot, bot_edict);
   killer = &mock_edicts[3];
   mock_set_netname(killer, "Killer");
   bot.killer_edict = killer;
   bot.f_bot_spawn_time = gpGlobals->time - 5.0; // only 5s alive
   bot_whine_count = 1;
   safe_strcopy(bot_whine[0].text, sizeof(bot_whine[0].text), "test");
   bot_whine[0].can_modify = FALSE;
   BotChatWhine(bot);
   ASSERT_INT(bot.b_bot_say, FALSE);
   PASS();

   TEST("skip: percent=0");
   reset_chat_state();
   bot_edict = &mock_edicts[1];
   setup_bot_for_chat_test(bot, bot_edict);
   bot.whine_percent = 0;
   killer = &mock_edicts[3];
   mock_set_netname(killer, "Killer");
   bot.killer_edict = killer;
   bot.f_bot_spawn_time = 0;
   bot_whine_count = 1;
   safe_strcopy(bot_whine[0].text, sizeof(bot_whine[0].text), "test");
   bot_whine[0].can_modify = FALSE;
   BotChatWhine(bot);
   ASSERT_INT(bot.b_bot_say, FALSE);
   PASS();

   TEST("success: sets b_bot_say and message");
   reset_chat_state();
   bot_edict = &mock_edicts[1];
   setup_bot_for_chat_test(bot, bot_edict);
   killer = &mock_edicts[3];
   mock_set_netname(killer, "Killer");
   bot.killer_edict = killer;
   bot.f_bot_spawn_time = 0; // spawned long ago (spawn_time + 15 <= time=10)
   // Wait: f_bot_spawn_time=0, gpGlobals->time=10, condition: 0+15 <= 10 -> FALSE
   // Need spawn_time such that spawn_time + 15 <= time
   // time is 10.0 from mock_reset, so spawn_time must be <= -5.0
   // Let's set time higher instead
   gpGlobals->time = 100.0;
   bot.f_bot_spawn_time = 0;
   bot_whine_count = 1;
   safe_strcopy(bot_whine[0].text, sizeof(bot_whine[0].text), "why %n");
   bot_whine[0].can_modify = FALSE;
   BotChatWhine(bot);
   ASSERT_INT(bot.b_bot_say, TRUE);
   ASSERT_TRUE(bot.f_bot_say > gpGlobals->time);
   ASSERT_STR(bot.bot_say_msg, "why Killer");
   PASS();

   TEST("killer netname NULL -> uses 'NULL'");
   reset_chat_state();
   gpGlobals->time = 100.0;
   bot_edict = &mock_edicts[1];
   setup_bot_for_chat_test(bot, bot_edict);
   killer = &mock_edicts[3];
   // Don't set netname -> stays 0
   bot.killer_edict = killer;
   bot.f_bot_spawn_time = 0;
   bot_whine_count = 1;
   safe_strcopy(bot_whine[0].text, sizeof(bot_whine[0].text), "curse %n");
   bot_whine[0].can_modify = FALSE;
   BotChatWhine(bot);
   ASSERT_STR(bot.bot_say_msg, "curse NULL");
   PASS();

   TEST("can_modify=TRUE with lowercase");
   reset_chat_state();
   gpGlobals->time = 100.0;
   bot_chat_lower_percent = 100;
   bot_edict = &mock_edicts[1];
   setup_bot_for_chat_test(bot, bot_edict);
   killer = &mock_edicts[3];
   mock_set_netname(killer, "Killer");
   bot.killer_edict = killer;
   bot.f_bot_spawn_time = 0;
   bot_whine_count = 1;
   safe_strcopy(bot_whine[0].text, sizeof(bot_whine[0].text), "WHY ME");
   bot_whine[0].can_modify = TRUE;
   BotChatWhine(bot);
   ASSERT_INT(bot.b_bot_say, TRUE);
   ASSERT_STR(bot.bot_say_msg, "why me");
   PASS();

   TEST("recent message deduplication loop");
   reset_chat_state();
   gpGlobals->time = 100.0;
   bot_edict = &mock_edicts[1];
   killer = &mock_edicts[3];
   mock_set_netname(killer, "Killer");
   // Load 6 whine messages
   bot_whine_count = 6;
   for (int i = 0; i < 6; i++)
   {
      char msg[16];
      snprintf(msg, sizeof(msg), "whine%d", i);
      safe_strcopy(bot_whine[i].text, sizeof(bot_whine[i].text), msg);
      bot_whine[i].can_modify = FALSE;
   }
   for (int i = 0; i < 7; i++)
   {
      fast_random_seed(i * 37 + 11);
      setup_bot_for_chat_test(bot, bot_edict);
      bot.killer_edict = killer;
      bot.f_bot_spawn_time = 0;
      BotChatWhine(bot);
      ASSERT_INT(bot.b_bot_say, TRUE);
   }
   PASS();

   return 0;
}

// ============================================================
// BotChatTalk tests
// ============================================================

static int test_BotChatTalk(void)
{
   bot_t bot;
   edict_t *bot_edict;

   printf("BotChatTalk:\n");

   TEST("skip: already chatting");
   reset_chat_state();
   bot_edict = &mock_edicts[1];
   setup_bot_for_chat_test(bot, bot_edict);
   bot.b_bot_say = TRUE;
   bot.f_bot_say = gpGlobals->time + 5.0;
   bot_chat_count = 1;
   safe_strcopy(bot_chat[0].text, sizeof(bot_chat[0].text), "test");
   bot_chat[0].can_modify = FALSE;
   BotChatTalk(bot);
   ASSERT_STR(bot.bot_say_msg, "");
   PASS();

   TEST("skip: no chat messages");
   reset_chat_state();
   bot_edict = &mock_edicts[1];
   setup_bot_for_chat_test(bot, bot_edict);
   BotChatTalk(bot);
   ASSERT_INT(bot.b_bot_say, FALSE);
   PASS();

   TEST("skip: cooldown active (f_bot_chat_time >= time)");
   reset_chat_state();
   bot_edict = &mock_edicts[1];
   setup_bot_for_chat_test(bot, bot_edict);
   bot.f_bot_chat_time = gpGlobals->time + 100.0;
   bot_chat_count = 1;
   safe_strcopy(bot_chat[0].text, sizeof(bot_chat[0].text), "test");
   bot_chat[0].can_modify = FALSE;
   BotChatTalk(bot);
   ASSERT_INT(bot.b_bot_say, FALSE);
   PASS();

   TEST("skip: percent=0");
   reset_chat_state();
   bot_edict = &mock_edicts[1];
   setup_bot_for_chat_test(bot, bot_edict);
   bot.chat_percent = 0;
   bot_chat_count = 1;
   safe_strcopy(bot_chat[0].text, sizeof(bot_chat[0].text), "test");
   bot_chat[0].can_modify = FALSE;
   BotChatTalk(bot);
   // Even though percent check fails, f_bot_chat_time still set to +30s
   ASSERT_INT(bot.b_bot_say, FALSE);
   PASS();

   TEST("success: sets message and 30s cooldown");
   reset_chat_state();
   bot_edict = &mock_edicts[1];
   setup_bot_for_chat_test(bot, bot_edict);
   bot_chat_count = 1;
   safe_strcopy(bot_chat[0].text, sizeof(bot_chat[0].text), "hello");
   bot_chat[0].can_modify = FALSE;
   BotChatTalk(bot);
   ASSERT_INT(bot.b_bot_say, TRUE);
   ASSERT_STR(bot.bot_say_msg, "hello");
   // Cooldown: f_bot_chat_time should be time + 30.0
   ASSERT_TRUE(bot.f_bot_chat_time > gpGlobals->time);
   PASS();

   TEST("can_modify=TRUE with lowercase");
   reset_chat_state();
   bot_chat_lower_percent = 100;
   bot_edict = &mock_edicts[1];
   setup_bot_for_chat_test(bot, bot_edict);
   bot_chat_count = 1;
   safe_strcopy(bot_chat[0].text, sizeof(bot_chat[0].text), "HELLO WORLD");
   bot_chat[0].can_modify = TRUE;
   BotChatTalk(bot);
   ASSERT_INT(bot.b_bot_say, TRUE);
   ASSERT_STR(bot.bot_say_msg, "hello world");
   PASS();

   TEST("recent message deduplication loop");
   reset_chat_state();
   bot_edict = &mock_edicts[1];
   // Load 6 chat messages
   bot_chat_count = 6;
   for (int i = 0; i < 6; i++)
   {
      char msg[16];
      snprintf(msg, sizeof(msg), "chat%d", i);
      safe_strcopy(bot_chat[i].text, sizeof(bot_chat[i].text), msg);
      bot_chat[i].can_modify = FALSE;
   }
   for (int i = 0; i < 7; i++)
   {
      fast_random_seed(i * 41 + 13);
      setup_bot_for_chat_test(bot, bot_edict);
      // Advance time past 30s cooldown for each call
      gpGlobals->time += 31.0;
      BotChatTalk(bot);
      ASSERT_INT(bot.b_bot_say, TRUE);
   }
   PASS();

   return 0;
}

// ============================================================
// BotChatEndGame tests
// ============================================================

static int test_BotChatEndGame(void)
{
   bot_t bot;
   edict_t *bot_edict;

   printf("BotChatEndGame:\n");

   TEST("skip: no endgame messages");
   reset_chat_state();
   bot_edict = &mock_edicts[1];
   setup_bot_for_chat_test(bot, bot_edict);
   BotChatEndGame(bot);
   ASSERT_INT(bot.b_bot_say, FALSE);
   PASS();

   TEST("skip: percent=0");
   reset_chat_state();
   bot_edict = &mock_edicts[1];
   setup_bot_for_chat_test(bot, bot_edict);
   bot.endgame_percent = 0;
   bot_endgame_count = 1;
   safe_strcopy(bot_endgame[0].text, sizeof(bot_endgame[0].text), "gg");
   bot_endgame[0].can_modify = FALSE;
   BotChatEndGame(bot);
   ASSERT_INT(bot.b_bot_say, FALSE);
   PASS();

   TEST("success: sets message");
   reset_chat_state();
   bot_edict = &mock_edicts[1];
   setup_bot_for_chat_test(bot, bot_edict);
   bot_endgame_count = 1;
   safe_strcopy(bot_endgame[0].text, sizeof(bot_endgame[0].text), "gg wp");
   bot_endgame[0].can_modify = FALSE;
   BotChatEndGame(bot);
   ASSERT_INT(bot.b_bot_say, TRUE);
   ASSERT_TRUE(bot.f_bot_say > gpGlobals->time);
   ASSERT_STR(bot.bot_say_msg, "gg wp");
   PASS();

   TEST("no already-chatting guard (unlike other chat funcs)");
   reset_chat_state();
   bot_edict = &mock_edicts[1];
   setup_bot_for_chat_test(bot, bot_edict);
   bot.b_bot_say = TRUE;
   bot.f_bot_say = gpGlobals->time + 5.0;
   bot_endgame_count = 1;
   safe_strcopy(bot_endgame[0].text, sizeof(bot_endgame[0].text), "gg");
   bot_endgame[0].can_modify = FALSE;
   BotChatEndGame(bot);
   // BotChatEndGame does NOT check b_bot_say, so it overwrites
   ASSERT_INT(bot.b_bot_say, TRUE);
   ASSERT_STR(bot.bot_say_msg, "gg");
   PASS();

   TEST("can_modify=TRUE with lowercase");
   reset_chat_state();
   bot_chat_lower_percent = 100;
   bot_edict = &mock_edicts[1];
   setup_bot_for_chat_test(bot, bot_edict);
   bot_endgame_count = 1;
   safe_strcopy(bot_endgame[0].text, sizeof(bot_endgame[0].text), "GOOD GAME");
   bot_endgame[0].can_modify = TRUE;
   BotChatEndGame(bot);
   ASSERT_INT(bot.b_bot_say, TRUE);
   ASSERT_STR(bot.bot_say_msg, "good game");
   PASS();

   TEST("recent message deduplication loop");
   reset_chat_state();
   bot_edict = &mock_edicts[1];
   // Load 6 endgame messages
   bot_endgame_count = 6;
   for (int i = 0; i < 6; i++)
   {
      char msg[16];
      snprintf(msg, sizeof(msg), "endgame%d", i);
      safe_strcopy(bot_endgame[i].text, sizeof(bot_endgame[i].text), msg);
      bot_endgame[i].can_modify = FALSE;
   }
   for (int i = 0; i < 7; i++)
   {
      fast_random_seed(i * 43 + 17);
      setup_bot_for_chat_test(bot, bot_edict);
      BotChatEndGame(bot);
      ASSERT_INT(bot.b_bot_say, TRUE);
   }
   PASS();

   return 0;
}

// ============================================================
// main
// ============================================================

int main()
{
   int rc = 0;

   rc |= test_BotTrimBlanks();
   rc |= test_BotChatTrimTag();
   rc |= test_BotDropCharacter();
   rc |= test_BotSwapCharacter();
   rc |= test_BotChatName();
   rc |= test_BotChatText();
   rc |= test_BotChatGetPlayers();
   rc |= test_BotChatFillInName();
   rc |= test_LoadBotChat();
   rc |= test_BotChatTaunt();
   rc |= test_BotChatWhine();
   rc |= test_BotChatTalk();
   rc |= test_BotChatEndGame();

   cleanup_temp_dir();

   printf("\n%d/%d tests passed\n", tests_passed, tests_run);
   return rc ? 1 : (tests_passed < tests_run ? 1 : 0);
}
