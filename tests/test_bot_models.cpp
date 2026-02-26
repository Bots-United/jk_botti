//
// JK_Botti - unit tests for bot_models.cpp
//
// test_bot_models.cpp
//
// Uses the #include-the-.cpp approach to access static FindDirectory directly.
//

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <ftw.h>

// Include the source file under test (brings in all its headers + static functions)
#include "../bot_models.cpp"

// Test infrastructure
#include "engine_mock.h"
#include "test_common.h"

// Suppress warn_unused_result for chdir/getcwd (test-only, not safety-critical)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-result"

// ============================================================
// Temp directory helpers
// ============================================================

static char test_tmpdir[512];
static char saved_cwd[512];

static int nftw_remove_cb(const char *fpath, const struct stat *sb,
                          int typeflag, struct FTW *ftwbuf)
{
   (void)sb; (void)typeflag; (void)ftwbuf;
   return remove(fpath);
}

static void cleanup_tmpdir(void)
{
   if (test_tmpdir[0])
   {
      nftw(test_tmpdir, nftw_remove_cb, 64, FTW_DEPTH | FTW_PHYS);
      test_tmpdir[0] = 0;
   }
}

static void create_tmpdir(void)
{
   cleanup_tmpdir();
   snprintf(test_tmpdir, sizeof(test_tmpdir), "/tmp/jk_botti_models_XXXXXX");
   if (!mkdtemp(test_tmpdir))
   {
      perror("mkdtemp");
      exit(1);
   }
}

// Create <basedir>/models/player/<modelname>/<modelname>.mdl
static void create_model_dir(const char *basedir, const char *modelname)
{
   char path[1536];

   snprintf(path, sizeof(path), "%s/models", basedir);
   mkdir(path, 0755);
   snprintf(path, sizeof(path), "%s/models/player", basedir);
   mkdir(path, 0755);
   snprintf(path, sizeof(path), "%s/models/player/%s", basedir, modelname);
   mkdir(path, 0755);
   snprintf(path, sizeof(path), "%s/models/player/%s/%s.mdl",
            basedir, modelname, modelname);
   FILE *fp = fopen(path, "w");
   if (fp)
   {
      fputs("fake_mdl", fp);
      fclose(fp);
   }
}

// Create <basedir>/models/player/<dirname>/ without a .mdl file
static void create_model_dir_no_mdl(const char *basedir, const char *dirname)
{
   char path[1536];

   snprintf(path, sizeof(path), "%s/models", basedir);
   mkdir(path, 0755);
   snprintf(path, sizeof(path), "%s/models/player", basedir);
   mkdir(path, 0755);
   snprintf(path, sizeof(path), "%s/models/player/%s", basedir, dirname);
   mkdir(path, 0755);
}

// ============================================================
// Mock GetGameDir
//
// GetGameDir() calls GET_GAME_DIR (pfnGetGameDir) then strips the
// path to the last component.  So "/tmp/xxx/valve" -> "valve".
// LoadBotModels then uses "valve/models/player" as a relative path.
// We chdir to the temp root so the relative path resolves.
// ============================================================

static char mock_gamedir_path[512];

static void mock_pfnGetGameDir(char *szGetGameDir)
{
   strcpy(szGetGameDir, mock_gamedir_path);
}

// Setup mock + chdir to tmpdir so relative game dirs resolve
static void setup_mock_with_gamedir(const char *gamedir_name)
{
   mock_reset();
   g_engfuncs.pfnGetGameDir = mock_pfnGetGameDir;

   // GetGameDir strips to last path component, so we just pass the name directly
   // (no path prefix needed since GetGameDir would strip it anyway)
   snprintf(mock_gamedir_path, sizeof(mock_gamedir_path), "%s", gamedir_name);

   // chdir to tmpdir so relative paths resolve
   chdir(test_tmpdir);
}

static void restore_cwd(void)
{
   chdir(saved_cwd);
}

#pragma GCC diagnostic pop

// ============================================================
// Tests
// ============================================================

static int test_hldm_defaults(void)
{
   TEST("LoadBotModels: HLDM loads 10 default skins");

   create_tmpdir();
   setup_mock_with_gamedir("nonexistent");

   submod_id = SUBMOD_HLDM;
   LoadBotModels();

   restore_cwd();

   ASSERT_INT(number_skins, 10);

   // Verify first and last default model names
   ASSERT_STR(bot_skins[0].model_name, "barney");
   ASSERT_STR(bot_skins[0].bot_name, "Barney");
   ASSERT_STR(bot_skins[9].model_name, "zombie");
   ASSERT_STR(bot_skins[9].bot_name, "Zombie");

   // Verify all default skins are not marked as used
   for (int i = 0; i < MAX_SKINS; i++)
      ASSERT_INT(bot_skins[i].skin_used, FALSE);

   cleanup_tmpdir();
   PASS();
   return 0;
}

static int test_op4_defaults(void)
{
   TEST("LoadBotModels: OP4 loads 20 default skins");

   create_tmpdir();
   setup_mock_with_gamedir("nonexistent");

   submod_id = SUBMOD_OP4;
   LoadBotModels();

   restore_cwd();

   ASSERT_INT(number_skins, 20);

   // Verify HLDM defaults still present
   ASSERT_STR(bot_skins[0].model_name, "barney");
   ASSERT_STR(bot_skins[9].model_name, "zombie");

   // Verify OP4/Gearbox additions
   ASSERT_STR(bot_skins[10].model_name, "beret");
   ASSERT_STR(bot_skins[10].bot_name, "Beret");
   ASSERT_STR(bot_skins[19].model_name, "tower");
   ASSERT_STR(bot_skins[19].bot_name, "Tower");

   cleanup_tmpdir();
   PASS();
   return 0;
}

static int test_custom_model_added(void)
{
   TEST("LoadBotModels: custom model dir with .mdl is added");

   create_tmpdir();

   // Create gamedir with model subdirectory
   char gamedir[1024];
   snprintf(gamedir, sizeof(gamedir), "%s/valve", test_tmpdir);
   mkdir(gamedir, 0755);
   create_model_dir(gamedir, "mymodel");

   setup_mock_with_gamedir("valve");

   submod_id = SUBMOD_HLDM;
   LoadBotModels();

   restore_cwd();

   // Should have 10 defaults + 1 custom
   ASSERT_INT(number_skins, 11);
   ASSERT_STR(bot_skins[10].model_name, "mymodel");
   ASSERT_STR(bot_skins[10].bot_name, "Mymodel");

   cleanup_tmpdir();
   PASS();
   return 0;
}

static int test_duplicate_model_skipped(void)
{
   TEST("LoadBotModels: duplicate model name is skipped");

   create_tmpdir();

   char gamedir[1024];
   snprintf(gamedir, sizeof(gamedir), "%s/valve", test_tmpdir);
   mkdir(gamedir, 0755);

   // Create a model dir with the same name as a default skin (barney)
   create_model_dir(gamedir, "barney");

   setup_mock_with_gamedir("valve");

   submod_id = SUBMOD_HLDM;
   LoadBotModels();

   restore_cwd();

   // Should still be 10 - duplicate "barney" is skipped
   ASSERT_INT(number_skins, 10);

   cleanup_tmpdir();
   PASS();
   return 0;
}

static int test_no_mdl_not_added(void)
{
   TEST("LoadBotModels: dir without .mdl file is not added");

   create_tmpdir();

   char gamedir[1024];
   snprintf(gamedir, sizeof(gamedir), "%s/valve", test_tmpdir);
   mkdir(gamedir, 0755);

   // Create directory without .mdl
   create_model_dir_no_mdl(gamedir, "nomdlmodel");

   setup_mock_with_gamedir("valve");

   submod_id = SUBMOD_HLDM;
   LoadBotModels();

   restore_cwd();

   // Should still be 10 defaults - no custom added
   ASSERT_INT(number_skins, 10);

   cleanup_tmpdir();
   PASS();
   return 0;
}

static int test_dirname_lowercased(void)
{
   TEST("LoadBotModels: dirname lowercased, bot_name first char upper");

   create_tmpdir();

   char gamedir[1024];
   snprintf(gamedir, sizeof(gamedir), "%s/valve", test_tmpdir);
   mkdir(gamedir, 0755);

   // Create model with mixed case name
   create_model_dir(gamedir, "MyMIXEDcase");

   setup_mock_with_gamedir("valve");

   submod_id = SUBMOD_HLDM;
   LoadBotModels();

   restore_cwd();

   // Model name should be all lowercase
   ASSERT_INT(number_skins, 11);
   ASSERT_STR(bot_skins[10].model_name, "mymixedcase");
   // Bot name should have first char uppercase, rest lowercase
   ASSERT_STR(bot_skins[10].bot_name, "Mymixedcase");

   cleanup_tmpdir();
   PASS();
   return 0;
}

static int test_fallback_to_valve(void)
{
   TEST("LoadBotModels: falls back to valve/models/player");

   create_tmpdir();

   // Create a gamedir that does NOT have models/player
   char gamedir[1024];
   snprintf(gamedir, sizeof(gamedir), "%s/mygame", test_tmpdir);
   mkdir(gamedir, 0755);
   // Do NOT create models/player under mygame

   // Create valve/models/player with a custom model (fallback path)
   char valvedir[1024];
   snprintf(valvedir, sizeof(valvedir), "%s/valve", test_tmpdir);
   mkdir(valvedir, 0755);
   create_model_dir(valvedir, "fallbackmodel");

   setup_mock_with_gamedir("mygame");

   submod_id = SUBMOD_HLDM;
   LoadBotModels();

   restore_cwd();

   // Should have 10 defaults + 1 from valve fallback
   ASSERT_INT(number_skins, 11);
   ASSERT_STR(bot_skins[10].model_name, "fallbackmodel");
   ASSERT_STR(bot_skins[10].bot_name, "Fallbackmodel");

   cleanup_tmpdir();
   PASS();
   return 0;
}

static int test_max_skins_limit(void)
{
   TEST("LoadBotModels: MAX_SKINS limit is respected");

   create_tmpdir();

   char gamedir[1024];
   snprintf(gamedir, sizeof(gamedir), "%s/valve", test_tmpdir);
   mkdir(gamedir, 0755);

   // Create enough models to exceed MAX_SKINS (200).
   // HLDM starts with 10 defaults, so we need 191+ custom models to hit the limit.
   char modelname[64];
   for (int i = 0; i < 195; i++)
   {
      snprintf(modelname, sizeof(modelname), "model%03d", i);
      create_model_dir(gamedir, modelname);
   }

   setup_mock_with_gamedir("valve");

   submod_id = SUBMOD_HLDM;
   LoadBotModels();

   restore_cwd();

   // Should be capped at MAX_SKINS (200)
   ASSERT_TRUE(number_skins <= MAX_SKINS);
   ASSERT_INT(number_skins, MAX_SKINS);

   // Verify defaults are still present
   ASSERT_STR(bot_skins[0].model_name, "barney");

   cleanup_tmpdir();
   PASS();
   return 0;
}

static int test_multiple_custom_models(void)
{
   TEST("LoadBotModels: multiple custom models all added");

   create_tmpdir();

   char gamedir[1024];
   snprintf(gamedir, sizeof(gamedir), "%s/valve", test_tmpdir);
   mkdir(gamedir, 0755);

   create_model_dir(gamedir, "alpha");
   create_model_dir(gamedir, "beta");
   create_model_dir(gamedir, "gamma");

   setup_mock_with_gamedir("valve");

   submod_id = SUBMOD_HLDM;
   LoadBotModels();

   restore_cwd();

   // Should have 10 defaults + 3 custom = 13
   ASSERT_INT(number_skins, 13);

   // Find each custom model in the skins list (order depends on readdir)
   int found_alpha = 0, found_beta = 0, found_gamma = 0;
   for (int i = 10; i < number_skins; i++)
   {
      if (strcmp(bot_skins[i].model_name, "alpha") == 0) found_alpha = 1;
      if (strcmp(bot_skins[i].model_name, "beta") == 0) found_beta = 1;
      if (strcmp(bot_skins[i].model_name, "gamma") == 0) found_gamma = 1;
   }
   ASSERT_TRUE(found_alpha);
   ASSERT_TRUE(found_beta);
   ASSERT_TRUE(found_gamma);

   cleanup_tmpdir();
   PASS();
   return 0;
}

static int test_skin_used_cleared(void)
{
   TEST("LoadBotModels: all skin_used flags cleared on load");

   create_tmpdir();
   setup_mock_with_gamedir("nonexistent");

   // Set some skin_used flags before calling LoadBotModels
   for (int i = 0; i < MAX_SKINS; i++)
      bot_skins[i].skin_used = TRUE;

   submod_id = SUBMOD_HLDM;
   LoadBotModels();

   restore_cwd();

   // All should be cleared
   for (int i = 0; i < MAX_SKINS; i++)
      ASSERT_INT(bot_skins[i].skin_used, FALSE);

   cleanup_tmpdir();
   PASS();
   return 0;
}

// ============================================================
// main
// ============================================================

int main(void)
{
   int fail = 0;

   // Save initial CWD for restoration between tests
   if (!getcwd(saved_cwd, sizeof(saved_cwd)))
   {
      perror("getcwd");
      return EXIT_FAILURE;
   }

   printf("test_bot_models:\n");
   fail |= test_hldm_defaults();
   fail |= test_op4_defaults();
   fail |= test_custom_model_added();
   fail |= test_duplicate_model_skipped();
   fail |= test_no_mdl_not_added();
   fail |= test_dirname_lowercased();
   fail |= test_fallback_to_valve();
   fail |= test_max_skins_limit();
   fail |= test_multiple_custom_models();
   fail |= test_skin_used_cleared();

   printf("\n%d/%d tests passed\n", tests_passed, tests_run);

   // Final cleanup in case any test leaked
   cleanup_tmpdir();

   return fail ? EXIT_FAILURE : EXIT_SUCCESS;
}
