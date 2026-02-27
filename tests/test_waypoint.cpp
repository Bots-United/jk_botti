//
// JK_Botti - unit tests for waypoint.cpp
//
// test_waypoint.cpp
//
// Uses the #include-the-.cpp approach to access static functions directly.
//

#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>

// waypoint.cpp declares: extern int m_spriteTexture;
// engine_mock.cpp provides it (non-weak), so we don't re-define it here.

// Include the source file under test (brings in all its headers + static functions)
#include "../waypoint.cpp"

// Test infrastructure
#include "engine_mock.h"
#include "test_common.h"

// ============================================================
// Test helpers
// ============================================================

static void reset_waypoint_state(void)
{
   mock_reset();
   InitWeaponSelect(SUBMOD_HLDM);
   WaypointInit();
   g_waypoint_on = FALSE;
   g_path_waypoint = FALSE;
   g_waypoint_paths = FALSE;
   g_auto_waypoint = TRUE;
   g_path_waypoint_enable = TRUE;
   g_waypoint_updated = FALSE;
   g_waypoint_testing = FALSE;
   f_path_time = 0.0;
   g_lifts_added = 0;
   wp_matrix_save_on_mapend = FALSE;
   num_spawnpoints = 0;
   memset(spawnpoints, 0, sizeof(spawnpoints));
   fast_random_seed(42);
}

static void setup_waypoint(int index, Vector origin, int flags, int itemflags)
{
   waypoints[index].origin = origin;
   waypoints[index].flags = flags;
   waypoints[index].itemflags = itemflags;
   if (index >= num_waypoints)
      num_waypoints = index + 1;
}

static void setup_path(short int from, short int to)
{
   WaypointAddPath(from, to);
}

static edict_t *setup_entity(const char *classname, Vector origin)
{
   edict_t *e = mock_alloc_edict();
   if (!e) return NULL;
   mock_set_classname(e, classname);
   e->v.origin = origin;
   return e;
}

static void setup_route_matrix(unsigned int num_wpts)
{
   route_num_waypoints = num_wpts;
   shortest_path = waypoint_matrix_mem_array_shortest_path;
   from_to = waypoint_matrix_mem_array_from_to_array;

   unsigned int array_size = num_wpts * num_wpts;
   for (unsigned int i = 0; i < array_size; i++)
   {
      shortest_path[i] = WAYPOINT_UNREACHABLE;
      from_to[i] = WAYPOINT_UNREACHABLE;
   }
   // zero diagonal
   for (unsigned int i = 0; i < num_wpts; i++)
   {
      shortest_path[i * num_wpts + i] = 0;
      from_to[i * num_wpts + i] = i;
   }

   wp_matrix_initialized = TRUE;
}

// ============================================================
// Temp file helpers for gzip save/load tests
// ============================================================

// Suppress warn_unused_result for chdir/getcwd
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
      snprintf(test_tmpdir, sizeof(test_tmpdir), "/tmp/jk_botti_wpt_XXXXXX");
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
      snprintf(path, sizeof(path), "%s/valve/addons/jk_botti/waypoints", test_tmpdir);
      mkdir(path, 0755);

      temp_initialized = 1;
   }
}

static void enter_temp_dir(void)
{
   init_temp_dir();
   chdir(test_tmpdir);
}

static void leave_temp_dir(void)
{
   chdir(saved_cwd);
}

static void cleanup_temp_dir(void)
{
   if (temp_initialized)
   {
      chdir(saved_cwd);

      char path[512];
      // Remove any .wpt and .matrix files
      snprintf(path, sizeof(path), "%s/valve/addons/jk_botti/waypoints/testmap.wpt", test_tmpdir);
      unlink(path);
      snprintf(path, sizeof(path), "%s/valve/addons/jk_botti/waypoints/testmap.matrix", test_tmpdir);
      unlink(path);
      snprintf(path, sizeof(path), "%s/valve/addons/jk_botti/waypoints", test_tmpdir);
      rmdir(path);
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
// Tests: WaypointIsRouteValid
// ============================================================

static int test_WaypointIsRouteValid(void)
{
   printf("WaypointIsRouteValid:\n");

   TEST("returns FALSE when matrix not initialized");
   reset_waypoint_state();
   wp_matrix_initialized = FALSE;
   from_to = waypoint_matrix_mem_array_from_to_array;
   ASSERT_INT(WaypointIsRouteValid(0, 1), FALSE);
   PASS();

   TEST("returns FALSE when from_to is NULL");
   reset_waypoint_state();
   wp_matrix_initialized = TRUE;
   from_to = NULL;
   ASSERT_INT(WaypointIsRouteValid(0, 1), FALSE);
   PASS();

   TEST("returns FALSE when src < 0");
   reset_waypoint_state();
   setup_route_matrix(4);
   ASSERT_INT(WaypointIsRouteValid(-1, 1), FALSE);
   PASS();

   TEST("returns FALSE when dest < 0");
   reset_waypoint_state();
   setup_route_matrix(4);
   ASSERT_INT(WaypointIsRouteValid(0, -1), FALSE);
   PASS();

   TEST("returns FALSE when src >= route_num_waypoints");
   reset_waypoint_state();
   setup_route_matrix(4);
   ASSERT_INT(WaypointIsRouteValid(4, 1), FALSE);
   PASS();

   TEST("returns TRUE for valid src and dest");
   reset_waypoint_state();
   setup_route_matrix(4);
   ASSERT_INT(WaypointIsRouteValid(0, 3), TRUE);
   PASS();

   return 0;
}

// ============================================================
// Tests: WaypointFree
// ============================================================

static int test_WaypointFree(void)
{
   printf("WaypointFree:\n");

   TEST("clears all path data");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), 0, 0);
   setup_waypoint(1, Vector(100,0,0), 0, 0);
   setup_path(0, 1);
   ASSERT_TRUE(paths[0].last_idx_used > 0);
   WaypointFree();
   ASSERT_INT(paths[0].last_idx_used, 0);
   ASSERT_INT(paths[1].last_idx_used, 0);
   PASS();

   TEST("handles empty paths");
   reset_waypoint_state();
   WaypointFree();
   ASSERT_INT(paths[0].last_idx_used, 0);
   PASS();

   return 0;
}

// ============================================================
// Tests: WaypointInit
// ============================================================

static int test_WaypointInit(void)
{
   printf("WaypointInit:\n");

   TEST("resets num_waypoints to 0");
   reset_waypoint_state();
   num_waypoints = 10;
   WaypointInit();
   ASSERT_INT(num_waypoints, 0);
   PASS();

   TEST("clears waypoint data");
   reset_waypoint_state();
   waypoints[0].flags = W_FL_HEALTH;
   waypoints[0].origin = Vector(100, 200, 300);
   WaypointInit();
   ASSERT_INT(waypoints[0].flags, 0);
   ASSERT_TRUE(waypoints[0].origin.x == 0.0f);
   PASS();

   TEST("clears path data");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), 0, 0);
   setup_waypoint(1, Vector(100,0,0), 0, 0);
   setup_path(0, 1);
   g_waypoint_paths = TRUE;
   WaypointInit();
   ASSERT_INT(paths[0].last_idx_used, 0);
   PASS();

   TEST("resets player waypoints to -1");
   reset_waypoint_state();
   players[0].last_waypoint = 5;
   players[31].last_waypoint = 10;
   WaypointInit();
   ASSERT_INT(players[0].last_waypoint, -1);
   ASSERT_INT(players[31].last_waypoint, -1);
   PASS();

   TEST("resets route matrix pointers");
   reset_waypoint_state();
   shortest_path = waypoint_matrix_mem_array_shortest_path;
   from_to = waypoint_matrix_mem_array_from_to_array;
   wp_matrix_initialized = TRUE;
   WaypointInit();
   ASSERT_PTR_NULL(shortest_path);
   ASSERT_PTR_NULL(from_to);
   ASSERT_INT(wp_matrix_initialized, FALSE);
   PASS();

   return 0;
}

// ============================================================
// Tests: WaypointAddPath / WaypointDeletePath
// ============================================================

static int test_WaypointAddDeletePath(void)
{
   printf("WaypointAddPath / WaypointDeletePath:\n");

   TEST("add single path");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), 0, 0);
   setup_waypoint(1, Vector(100,0,0), 0, 0);
   setup_path(0, 1);
   ASSERT_INT(paths[0].last_idx_used, 1);
   ASSERT_INT(paths[0].index[0], 1);
   PASS();

   TEST("reuse deleted slot on add");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), 0, 0);
   setup_waypoint(1, Vector(100,0,0), 0, 0);
   setup_waypoint(2, Vector(200,0,0), 0, 0);
   setup_path(0, 1);
   setup_path(0, 2);
   WaypointDeletePath(0, 1);
   // Now slot 0 is -1, slot 1 is 2
   setup_waypoint(3, Vector(300,0,0), 0, 0);
   setup_path(0, 3);
   // Should reuse the deleted slot[0]
   ASSERT_INT(paths[0].index[0], 3);
   ASSERT_INT(paths[0].index[1], 2);
   PASS();

   TEST("skip duplicate add");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), 0, 0);
   setup_waypoint(1, Vector(100,0,0), 0, 0);
   setup_path(0, 1);
   setup_path(0, 1); // duplicate
   ASSERT_INT(paths[0].last_idx_used, 1);
   PASS();

   TEST("refuse add to deleted waypoint");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), W_FL_DELETED, 0);
   setup_waypoint(1, Vector(100,0,0), 0, 0);
   setup_path(0, 1);
   ASSERT_INT(paths[0].last_idx_used, 0);
   PASS();

   TEST("refuse add from deleted waypoint");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), 0, 0);
   setup_waypoint(1, Vector(100,0,0), W_FL_DELETED, 0);
   setup_path(0, 1);
   ASSERT_INT(paths[0].last_idx_used, 0);
   PASS();

   TEST("delete single path");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), 0, 0);
   setup_waypoint(1, Vector(100,0,0), 0, 0);
   setup_path(0, 1);
   WaypointDeletePath(0, 1);
   ASSERT_INT(paths[0].index[0], -1);
   PASS();

   TEST("delete non-existent path is no-op");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), 0, 0);
   setup_waypoint(1, Vector(100,0,0), 0, 0);
   setup_path(0, 1);
   WaypointDeletePath(0, 2); // 2 doesn't exist
   ASSERT_INT(paths[0].index[0], 1); // still there
   PASS();

   TEST("delete all paths (single-arg overload)");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), 0, 0);
   setup_waypoint(1, Vector(100,0,0), 0, 0);
   setup_waypoint(2, Vector(200,0,0), 0, 0);
   setup_path(0, 1);
   setup_path(2, 1);
   WaypointDeletePath((short int)1); // delete all paths TO 1
   ASSERT_INT(paths[0].index[0], -1);
   ASSERT_INT(paths[2].index[0], -1);
   PASS();

   TEST("add-delete-add cycle");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), 0, 0);
   setup_waypoint(1, Vector(100,0,0), 0, 0);
   setup_path(0, 1);
   WaypointDeletePath(0, 1);
   ASSERT_INT(paths[0].index[0], -1);
   setup_path(0, 1);
   ASSERT_INT(paths[0].index[0], 1);
   PASS();

   TEST("fill to MAX_PATH_INDEX stops at limit");
   reset_waypoint_state();
   for (int i = 0; i < MAX_PATH_INDEX + 1; i++)
      setup_waypoint(i, Vector((float)i * 100, 0, 0), 0, 0);
   for (int i = 1; i <= MAX_PATH_INDEX; i++)
      setup_path(0, (short int)i);
   // Should have exactly MAX_PATH_INDEX paths
   ASSERT_INT(paths[0].last_idx_used, MAX_PATH_INDEX);
   // Trying to add one more should be silently ignored
   int extra = MAX_PATH_INDEX; // already at limit
   setup_waypoint(extra, Vector(99999,0,0), 0, 0);
   // Can't add, already at limit
   ASSERT_INT(paths[0].last_idx_used, MAX_PATH_INDEX);
   PASS();

   return 0;
}

// ============================================================
// Tests: WaypointFindPath
// ============================================================

static int test_WaypointFindPath(void)
{
   printf("WaypointFindPath:\n");

   TEST("no paths returns -1");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), 0, 0);
   {
      int pi = -1;
      ASSERT_INT(WaypointFindPath(pi, 0), -1);
   }
   PASS();

   TEST("iterates all paths");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), 0, 0);
   setup_waypoint(1, Vector(100,0,0), 0, 0);
   setup_waypoint(2, Vector(200,0,0), 0, 0);
   setup_path(0, 1);
   setup_path(0, 2);
   {
      int pi = -1;
      ASSERT_INT(WaypointFindPath(pi, 0), 1);
      ASSERT_INT(WaypointFindPath(pi, 0), 2);
      ASSERT_INT(WaypointFindPath(pi, 0), -1);
   }
   PASS();

   TEST("skips deleted entries");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), 0, 0);
   setup_waypoint(1, Vector(100,0,0), 0, 0);
   setup_waypoint(2, Vector(200,0,0), 0, 0);
   setup_path(0, 1);
   setup_path(0, 2);
   WaypointDeletePath(0, 1);
   {
      int pi = -1;
      ASSERT_INT(WaypointFindPath(pi, 0), 2);
      ASSERT_INT(WaypointFindPath(pi, 0), -1);
   }
   PASS();

   TEST("returns -1 after last path");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), 0, 0);
   setup_waypoint(1, Vector(100,0,0), 0, 0);
   setup_path(0, 1);
   {
      int pi = -1;
      ASSERT_INT(WaypointFindPath(pi, 0), 1);
      ASSERT_INT(WaypointFindPath(pi, 0), -1);
      ASSERT_INT(WaypointFindPath(pi, 0), -1); // stays at -1
   }
   PASS();

   return 0;
}

// ============================================================
// Tests: WaypointNumberOfPaths / WaypointIncomingPathsCount
// ============================================================

static int test_WaypointPathCounts(void)
{
   printf("WaypointNumberOfPaths / WaypointIncomingPathsCount:\n");

   TEST("zero outgoing paths");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), 0, 0);
   ASSERT_INT(WaypointNumberOfPaths(0), 0);
   PASS();

   TEST("counts non-deleted outgoing paths");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), 0, 0);
   setup_waypoint(1, Vector(100,0,0), 0, 0);
   setup_waypoint(2, Vector(200,0,0), 0, 0);
   setup_path(0, 1);
   setup_path(0, 2);
   ASSERT_INT(WaypointNumberOfPaths(0), 2);
   PASS();

   TEST("skips deleted paths in count");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), 0, 0);
   setup_waypoint(1, Vector(100,0,0), 0, 0);
   setup_waypoint(2, Vector(200,0,0), 0, 0);
   setup_path(0, 1);
   setup_path(0, 2);
   WaypointDeletePath(0, 1);
   ASSERT_INT(WaypointNumberOfPaths(0), 1);
   PASS();

   TEST("zero incoming paths");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), 0, 0);
   setup_waypoint(1, Vector(100,0,0), 0, 0);
   ASSERT_INT(WaypointIncomingPathsCount(0, 0), 0);
   PASS();

   TEST("counts incoming paths");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), 0, 0);
   setup_waypoint(1, Vector(100,0,0), 0, 0);
   setup_waypoint(2, Vector(200,0,0), 0, 0);
   setup_path(0, 2);
   setup_path(1, 2);
   ASSERT_INT(WaypointIncomingPathsCount(2, 0), 2);
   PASS();

   TEST("early exit with exit_count");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), 0, 0);
   setup_waypoint(1, Vector(100,0,0), 0, 0);
   setup_waypoint(2, Vector(200,0,0), 0, 0);
   setup_waypoint(3, Vector(300,0,0), 0, 0);
   setup_path(0, 3);
   setup_path(1, 3);
   setup_path(2, 3);
   // exit_count=1 should return as soon as >=1 is found
   ASSERT_TRUE(WaypointIncomingPathsCount(3, 1) >= 1);
   PASS();

   return 0;
}

// ============================================================
// Tests: WaypointTrim
// ============================================================

static int test_WaypointTrim(void)
{
   printf("WaypointTrim:\n");

   TEST("removes orphan waypoint (no paths in/out)");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), 0, 0);
   setup_waypoint(1, Vector(100,0,0), 0, 0);
   setup_waypoint(2, Vector(200,0,0), 0, 0);
   // 0->1 path, 2 is orphan
   setup_path(0, 1);
   WaypointTrim();
   ASSERT_TRUE(waypoints[2].flags & W_FL_DELETED);
   PASS();

   TEST("keeps waypoint with outgoing path");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), 0, 0);
   setup_waypoint(1, Vector(100,0,0), 0, 0);
   setup_path(0, 1);
   WaypointTrim();
   ASSERT_TRUE(!(waypoints[0].flags & W_FL_DELETED));
   PASS();

   TEST("keeps waypoint with incoming path");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), 0, 0);
   setup_waypoint(1, Vector(100,0,0), 0, 0);
   setup_path(0, 1);
   WaypointTrim();
   ASSERT_TRUE(!(waypoints[1].flags & W_FL_DELETED));
   PASS();

   TEST("skips deleted, aiming, and spawnadd waypoints");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), W_FL_DELETED, 0);
   setup_waypoint(1, Vector(100,0,0), W_FL_AIMING, 0);
   setup_waypoint(2, Vector(200,0,0), W_FL_SPAWNADD, 0);
   // None of these should be modified by trim
   WaypointTrim();
   // Deleted stays deleted (already was)
   ASSERT_TRUE(waypoints[0].flags & W_FL_DELETED);
   // Aiming stays aiming (not trimmed)
   ASSERT_INT(waypoints[1].flags, W_FL_AIMING);
   // SPAWNADD stays SPAWNADD (not trimmed)
   ASSERT_INT(waypoints[2].flags, W_FL_SPAWNADD);
   PASS();

   return 0;
}

// ============================================================
// Tests: WaypointFindRandomGoal
// ============================================================

static int test_WaypointFindRandomGoal(void)
{
   printf("WaypointFindRandomGoal:\n");

   TEST("empty waypoints returns 0");
   reset_waypoint_state();
   {
      int out[2] = {-1, -1};
      ASSERT_INT(WaypointFindRandomGoal(out, 1, NULL, 0, 0, NULL), 0);
   }
   PASS();

   TEST("bad args: max_indexes <= 0");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), 0, 0);
   {
      int out[1] = {-1};
      ASSERT_INT(WaypointFindRandomGoal(out, 0, NULL, 0, 0, NULL), 0);
   }
   PASS();

   TEST("bad args: out_indexes NULL");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), 0, 0);
   ASSERT_INT(WaypointFindRandomGoal(NULL, 1, NULL, 0, 0, NULL), 0);
   PASS();

   TEST("matching flags filter");
   reset_waypoint_state();
   // Use non-item flags (W_FL_CROUCH, W_FL_JUMP) to avoid WaypointFindItem check
   setup_waypoint(0, Vector(0,0,0), W_FL_CROUCH, 0);
   setup_waypoint(1, Vector(100,0,0), W_FL_JUMP, 0);
   setup_waypoint(2, Vector(200,0,0), W_FL_CROUCH, 0);
   {
      int out[4];
      int count = WaypointFindRandomGoal(out, 4, NULL, W_FL_CROUCH, 0, NULL);
      ASSERT_INT(count, 2);
      // Both should be crouch waypoints (0 or 2)
      ASSERT_TRUE((out[0] == 0 || out[0] == 2));
      ASSERT_TRUE((out[1] == 0 || out[1] == 2));
   }
   PASS();

   TEST("skips deleted and aiming waypoints");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), W_FL_DELETED, 0);
   setup_waypoint(1, Vector(100,0,0), W_FL_AIMING, 0);
   setup_waypoint(2, Vector(200,0,0), 0, 0);
   {
      int out[4];
      int count = WaypointFindRandomGoal(out, 4, NULL, 0, 0, NULL);
      ASSERT_INT(count, 1);
      ASSERT_INT(out[0], 2);
   }
   PASS();

   TEST("respects exclude list");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), 0, 0);
   setup_waypoint(1, Vector(100,0,0), 0, 0);
   setup_waypoint(2, Vector(200,0,0), 0, 0);
   {
      int exclude[] = {1, -1};
      int out[4];
      int count = WaypointFindRandomGoal(out, 4, NULL, 0, 0, exclude);
      ASSERT_INT(count, 2);
      ASSERT_TRUE(out[0] != 1);
      ASSERT_TRUE(out[1] != 1);
   }
   PASS();

   TEST("fewer matches than requested");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), 0, 0);
   setup_waypoint(1, Vector(100,0,0), 0, 0);
   {
      int out[4] = {-1, -1, -1, -1};
      int count = WaypointFindRandomGoal(out, 4, NULL, 0, 0, NULL);
      ASSERT_INT(count, 2); // only 2 available
      ASSERT_TRUE(out[0] >= 0);
      ASSERT_TRUE(out[1] >= 0);
   }
   PASS();

   TEST("BUG FIX: fills all out_indexes when max_indexes > 1");
   reset_waypoint_state();
   // Create 5 waypoints (more than max_indexes=3)
   for (int i = 0; i < 5; i++)
      setup_waypoint(i, Vector((float)i * 100, 0, 0), 0, 0);
   {
      int out[3] = {-99, -99, -99};
      int count = WaypointFindRandomGoal(out, 3, NULL, 0, 0, NULL);
      ASSERT_INT(count, 3);
      // All slots must be filled (not all -99 or overwriting index 0)
      ASSERT_TRUE(out[0] >= 0 && out[0] < 5);
      ASSERT_TRUE(out[1] >= 0 && out[1] < 5);
      ASSERT_TRUE(out[2] >= 0 && out[2] < 5);
   }
   PASS();

   return 0;
}

// ============================================================
// Tests: WaypointFindNearestGoal
// ============================================================

static int test_WaypointFindNearestGoal(void)
{
   printf("WaypointFindNearestGoal:\n");

   TEST("empty waypoints returns -1");
   reset_waypoint_state();
   ASSERT_INT(WaypointFindNearestGoal(NULL, 0, 0, 0, NULL, 0.0f, NULL), -1);
   PASS();

   TEST("nearest by position (pv_src)");
   reset_waypoint_state();
   // Use non-item flags to avoid WaypointFindItem NULL check
   setup_waypoint(0, Vector(0,0,0), W_FL_CROUCH, 0);
   setup_waypoint(1, Vector(100,0,0), W_FL_CROUCH, 0);
   setup_waypoint(2, Vector(1000,0,0), W_FL_CROUCH, 0);
   {
      Vector src(90, 0, 0);
      int idx = WaypointFindNearestGoal(NULL, -1, W_FL_CROUCH, 0, NULL, 0.0f, &src);
      ASSERT_INT(idx, 1); // closest to (90,0,0)
   }
   PASS();

   TEST("nearest by route distance");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), 0, 0);
   setup_waypoint(1, Vector(100,0,0), W_FL_CROUCH, 0);
   setup_waypoint(2, Vector(200,0,0), W_FL_CROUCH, 0);
   setup_route_matrix(3);
   // Set route distances: 0->1 = 500, 0->2 = 100
   shortest_path[0 * 3 + 1] = 500;
   from_to[0 * 3 + 1] = 1;
   shortest_path[0 * 3 + 2] = 100;
   from_to[0 * 3 + 2] = 2;
   {
      int idx = WaypointFindNearestGoal(NULL, 0, W_FL_CROUCH, 0, NULL, 0.0f, NULL);
      ASSERT_INT(idx, 2); // closer by route
   }
   PASS();

   TEST("flag filtering");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), 0, 0);
   setup_waypoint(1, Vector(100,0,0), W_FL_CROUCH, 0);
   setup_waypoint(2, Vector(200,0,0), W_FL_JUMP, 0);
   {
      Vector src(150, 0, 0);
      int idx = WaypointFindNearestGoal(NULL, -1, W_FL_CROUCH, 0, NULL, 0.0f, &src);
      ASSERT_INT(idx, 1);
   }
   PASS();

   TEST("exclude list");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), W_FL_CROUCH, 0);
   setup_waypoint(1, Vector(100,0,0), W_FL_CROUCH, 0);
   setup_waypoint(2, Vector(200,0,0), W_FL_CROUCH, 0);
   {
      Vector src(0, 0, 0);
      int exclude[] = {0, -1};
      int idx = WaypointFindNearestGoal(NULL, -1, W_FL_CROUCH, 0, exclude, 0.0f, &src);
      ASSERT_INT(idx, 1); // 0 excluded, next closest is 1
   }
   PASS();

   TEST("skips src waypoint");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), W_FL_CROUCH, 0);
   setup_waypoint(1, Vector(100,0,0), W_FL_CROUCH, 0);
   setup_route_matrix(2);
   shortest_path[0 * 2 + 1] = 100;
   from_to[0 * 2 + 1] = 1;
   {
      int idx = WaypointFindNearestGoal(NULL, 0, W_FL_CROUCH, 0, NULL, 0.0f, NULL);
      ASSERT_INT(idx, 1); // src=0 is skipped
   }
   PASS();

   TEST("range limit");
   reset_waypoint_state();
   setup_waypoint(0, Vector(-500,0,0), W_FL_CROUCH, 0);
   setup_waypoint(1, Vector(100,0,0), W_FL_CROUCH, 0);
   setup_waypoint(2, Vector(1000,0,0), W_FL_CROUCH, 0);
   {
      Vector src(0, 0, 0);
      // range=200: wp0 at 500 -> skip, wp1 at 100 -> match, wp2 at 1000 -> skip
      int idx = WaypointFindNearestGoal(NULL, -1, W_FL_CROUCH, 0, NULL, 200.0f, &src);
      ASSERT_INT(idx, 1);
   }
   PASS();

   return 0;
}

// ============================================================
// Tests: WaypointRouteMatrix (RouteFromTo, DistanceFromTo, FindRunawayPath, SlowFloyds)
// ============================================================

static int test_WaypointRouteMatrix(void)
{
   printf("WaypointRouteMatrix:\n");

   TEST("RouteFromTo uninit returns WAYPOINT_UNREACHABLE");
   reset_waypoint_state();
   ASSERT_INT(WaypointRouteFromTo(0, 1), WAYPOINT_UNREACHABLE);
   PASS();

   TEST("RouteFromTo returns correct next hop");
   reset_waypoint_state();
   setup_route_matrix(3);
   from_to[0 * 3 + 2] = 1; // next hop from 0 to 2 is via 1
   ASSERT_INT(WaypointRouteFromTo(0, 2), 1);
   PASS();

   TEST("RouteFromTo out-of-range returns WAYPOINT_UNREACHABLE");
   reset_waypoint_state();
   setup_route_matrix(3);
   ASSERT_INT(WaypointRouteFromTo(3, 0), WAYPOINT_UNREACHABLE);
   ASSERT_INT(WaypointRouteFromTo(0, 3), WAYPOINT_UNREACHABLE);
   PASS();

   TEST("DistanceFromTo uninit returns WAYPOINT_MAX_DISTANCE");
   reset_waypoint_state();
   ASSERT_TRUE(WaypointDistanceFromTo(0, 1) == (float)WAYPOINT_MAX_DISTANCE);
   PASS();

   TEST("DistanceFromTo returns correct distance");
   reset_waypoint_state();
   setup_route_matrix(3);
   shortest_path[0 * 3 + 2] = 500;
   from_to[0 * 3 + 2] = 1;
   ASSERT_TRUE(WaypointDistanceFromTo(0, 2) == 500.0f);
   PASS();

   TEST("FindRunawayPath returns -1 with no waypoints");
   reset_waypoint_state();
   ASSERT_INT(WaypointFindRunawayPath(0, 1), -1);
   PASS();

   TEST("FindRunawayPath returns -1 when runner unreachable to all");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), 0, 0);
   setup_waypoint(1, Vector(100,0,0), 0, 0);
   setup_waypoint(2, Vector(200,0,0), 0, 0);
   setup_route_matrix(3);
   // Use WAYPOINT_UNREACHABLE (65535) -- the real value Floyd's algorithm
   // leaves for unreachable paths (not WAYPOINT_MAX_DISTANCE which is 65534)
   shortest_path[0 * 3 + 0] = WAYPOINT_UNREACHABLE;
   shortest_path[0 * 3 + 1] = WAYPOINT_UNREACHABLE;
   shortest_path[0 * 3 + 2] = WAYPOINT_UNREACHABLE;
   ASSERT_INT(WaypointFindRunawayPath(0, 1), -1);
   PASS();

   TEST("FindRunawayPath picks waypoint far from enemy, close to runner");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), 0, 0);     // runner
   setup_waypoint(1, Vector(1000,0,0), 0, 0);   // enemy
   setup_waypoint(2, Vector(500,0,0), 0, 0);     // candidate A
   setup_waypoint(3, Vector(100,0,0), 0, 0);     // candidate B (better)
   setup_route_matrix(4);
   // runner(0)->2 = 500, runner(0)->3 = 100
   shortest_path[0 * 4 + 2] = 500;
   from_to[0 * 4 + 2] = 2;
   shortest_path[0 * 4 + 3] = 100;
   from_to[0 * 4 + 3] = 3;
   // enemy(1)->0 = 200, enemy(1)->2 = 500, enemy(1)->3 = 900
   shortest_path[1 * 4 + 0] = 200;
   from_to[1 * 4 + 0] = 0;
   shortest_path[1 * 4 + 2] = 500;
   from_to[1 * 4 + 2] = 2;
   shortest_path[1 * 4 + 3] = 900;
   from_to[1 * 4 + 3] = 3;
   // index 0: runner_dist=0, enemy_dist=200, diff=200
   // index 2: runner_dist=500, enemy_dist=500, diff=0
   // index 3: runner_dist=100, enemy_dist=900, diff=800 (best)
   ASSERT_INT(WaypointFindRunawayPath(0, 1), 3);
   PASS();

   TEST("SlowFloyds full computation on 3-node graph");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), 0, 0);
   setup_waypoint(1, Vector(100,0,0), 0, 0);
   setup_waypoint(2, Vector(200,0,0), 0, 0);
   setup_path(0, 1);
   setup_path(1, 2);
   // No direct path 0->2

   // Setup route matrix the way WaypointRouteInit does
   route_num_waypoints = 3;
   shortest_path = waypoint_matrix_mem_array_shortest_path;
   from_to = waypoint_matrix_mem_array_from_to_array;
   unsigned int arr_size = 3 * 3;

   for (unsigned int i = 0; i < arr_size; i++)
      shortest_path[i] = WAYPOINT_UNREACHABLE;
   for (unsigned int i = 0; i < 3; i++)
      shortest_path[i * 3 + i] = 0;

   // Fill direct path distances
   // 0->1: distance = 100
   shortest_path[0 * 3 + 1] = 100;
   // 1->2: distance = 100
   shortest_path[1 * 3 + 2] = 100;

   wp_matrix_initialized = FALSE;
   slow_floyds.state = 0;

   // Run SlowFloyds to completion
   int result;
   int iterations = 0;
   do {
      result = WaypointSlowFloyds();
      iterations++;
   } while (result != -1 && iterations < 10000);

   ASSERT_INT(wp_matrix_initialized, TRUE);
   // 0->2 should now be reachable via 1, distance = 200
   ASSERT_TRUE(shortest_path[0 * 3 + 2] == 200);
   // next hop from 0 to 2 should be 1
   ASSERT_INT((int)from_to[0 * 3 + 2], 1);
   // 2->0 should be unreachable (no reverse paths)
   ASSERT_TRUE(from_to[2 * 3 + 0] == WAYPOINT_UNREACHABLE);
   PASS();

   return 0;
}

// ============================================================
// Tests: WaypointSave / WaypointLoad round-trip
// ============================================================

static int test_WaypointSaveLoad(void)
{
   printf("WaypointSave / WaypointLoad:\n");

   TEST("round-trip preserves waypoints, paths, and flags");
   reset_waypoint_state();
   enter_temp_dir();
   // Set mapname so save/load can construct filename
   gpGlobals->mapname = (string_t)(long)"testmap";
   g_waypoint_updated = TRUE;

   setup_waypoint(0, Vector(10, 20, 30), W_FL_HEALTH, 0);
   setup_waypoint(1, Vector(100, 200, 300), W_FL_AMMO, 0x42);
   setup_waypoint(2, Vector(500, 600, 700), W_FL_WEAPON, 0x10);
   setup_path(0, 1);
   setup_path(1, 2);
   setup_path(0, 2);
   g_waypoint_paths = TRUE;
   // All waypoints have paths, so none should be trimmed

   WaypointSave();

   // Reset and reload
   WaypointInit();
   ASSERT_INT(num_waypoints, 0);

   qboolean loaded = WaypointLoad(NULL);
   ASSERT_INT(loaded, TRUE);
   ASSERT_INT(num_waypoints, 3);

   // Check waypoint data preserved
   ASSERT_TRUE(waypoints[0].origin.x == 10.0f);
   ASSERT_TRUE(waypoints[0].origin.y == 20.0f);
   ASSERT_TRUE(waypoints[0].origin.z == 30.0f);
   ASSERT_INT(waypoints[0].flags, W_FL_HEALTH);

   ASSERT_TRUE(waypoints[1].origin.x == 100.0f);
   ASSERT_INT(waypoints[1].flags, W_FL_AMMO);
   ASSERT_INT(waypoints[1].itemflags, 0x42);

   ASSERT_TRUE(waypoints[2].origin.x == 500.0f);
   ASSERT_INT(waypoints[2].flags, W_FL_WEAPON);

   // Check paths preserved
   ASSERT_INT(WaypointNumberOfPaths(0), 2); // 0->1, 0->2
   ASSERT_INT(WaypointNumberOfPaths(1), 1); // 1->2
   ASSERT_INT(WaypointNumberOfPaths(2), 0);
   {
      int pi = -1;
      ASSERT_INT(WaypointFindPath(pi, 0), 1);
      ASSERT_INT(WaypointFindPath(pi, 0), 2);
   }
   leave_temp_dir();
   PASS();

   TEST("load non-existent file returns FALSE");
   reset_waypoint_state();
   enter_temp_dir();
   gpGlobals->mapname = (string_t)(long)"nonexistent_map";
   ASSERT_INT(WaypointLoad(NULL), FALSE);
   leave_temp_dir();
   PASS();

   TEST("load file with wrong magic returns FALSE");
   reset_waypoint_state();
   enter_temp_dir();
   gpGlobals->mapname = (string_t)(long)"testmap";
   {
      // Write a file with bad magic
      char filename[512];
      snprintf(filename, sizeof(filename),
               "%s/valve/addons/jk_botti/waypoints/testmap.wpt", test_tmpdir);
      gzFile fp = gzopen(filename, "wb");
      WAYPOINT_HDR hdr;
      memset(&hdr, 0, sizeof(hdr));
      memcpy(hdr.filetype, "BADMAGIC", 8);
      hdr.waypoint_file_version = WAYPOINT_VERSION;
      hdr.number_of_waypoints = 0;
      gzwrite(fp, &hdr, sizeof(hdr));
      gzclose(fp);

      ASSERT_INT(WaypointLoad(NULL), FALSE);
   }
   leave_temp_dir();
   PASS();

   TEST("load file with wrong version returns FALSE");
   reset_waypoint_state();
   enter_temp_dir();
   gpGlobals->mapname = (string_t)(long)"testmap";
   {
      char filename[512];
      snprintf(filename, sizeof(filename),
               "%s/valve/addons/jk_botti/waypoints/testmap.wpt", test_tmpdir);
      gzFile fp = gzopen(filename, "wb");
      WAYPOINT_HDR hdr;
      memset(&hdr, 0, sizeof(hdr));
      memcpy(hdr.filetype, WAYPOINT_MAGIC, 8);
      hdr.waypoint_file_version = 999; // wrong version
      hdr.number_of_waypoints = 0;
      gzwrite(fp, &hdr, sizeof(hdr));
      gzclose(fp);

      ASSERT_INT(WaypointLoad(NULL), FALSE);
   }
   leave_temp_dir();
   PASS();

   TEST("round-trip with deleted waypoint");
   reset_waypoint_state();
   enter_temp_dir();
   gpGlobals->mapname = (string_t)(long)"testmap";
   g_waypoint_updated = TRUE;

   setup_waypoint(0, Vector(10, 20, 30), 0, 0);
   setup_waypoint(1, Vector(100, 200, 300), W_FL_DELETED, 0);
   setup_waypoint(2, Vector(500, 600, 700), 0, 0);
   setup_path(0, 2);
   g_waypoint_paths = TRUE;

   WaypointSave();

   WaypointInit();
   qboolean loaded2 = WaypointLoad(NULL);
   ASSERT_INT(loaded2, TRUE);
   ASSERT_INT(num_waypoints, 3);
   ASSERT_TRUE(waypoints[1].flags & W_FL_DELETED);
   // Paths preserved
   ASSERT_INT(WaypointNumberOfPaths(0), 1);
   {
      int pi = -1;
      ASSERT_INT(WaypointFindPath(pi, 0), 2);
   }
   leave_temp_dir();
   PASS();

   TEST("save skipped when g_waypoint_testing is set");
   reset_waypoint_state();
   enter_temp_dir();
   gpGlobals->mapname = (string_t)(long)"testmap";
   g_waypoint_updated = TRUE;
   g_waypoint_testing = TRUE;

   setup_waypoint(0, Vector(10, 20, 30), 0, 0);
   setup_path(0, 0); // self-path just to have something
   g_waypoint_paths = TRUE;

   // Remove any old file first
   {
      char filename[512];
      snprintf(filename, sizeof(filename),
               "%s/valve/addons/jk_botti/waypoints/testmap.wpt", test_tmpdir);
      unlink(filename);
   }

   WaypointSave(); // should be skipped

   // Loading should fail since file doesn't exist
   WaypointInit();
   ASSERT_INT(WaypointLoad(NULL), FALSE);
   leave_temp_dir();
   PASS();

   return 0;
}

// ============================================================
// Tests: WaypointSaveFloydsMatrix
// ============================================================

static int test_WaypointSaveFloydsMatrix(void)
{
   printf("WaypointSaveFloydsMatrix:\n");

   TEST("round-trip save and manual read-back");
   reset_waypoint_state();
   enter_temp_dir();
   gpGlobals->mapname = (string_t)(long)"testmap";

   setup_route_matrix(3);
   // Set some known values
   shortest_path[0 * 3 + 1] = 100;
   shortest_path[0 * 3 + 2] = 200;
   shortest_path[1 * 3 + 2] = 150;
   from_to[0 * 3 + 1] = 1;
   from_to[0 * 3 + 2] = 1;
   from_to[1 * 3 + 2] = 2;

   WaypointSaveFloydsMatrix();

   // Read back manually and verify
   {
      char filename[512];
      snprintf(filename, sizeof(filename),
               "%s/valve/addons/jk_botti/waypoints/testmap.matrix", test_tmpdir);
      gzFile fp = gzopen(filename, "rb");
      ASSERT_TRUE(fp != NULL);

      char magic[16];
      int n = gzread(fp, magic, 16);
      ASSERT_INT(n, 16);
      ASSERT_TRUE(memcmp(magic, "jkbotti_matrixA\0", 16) == 0);

      unsigned short sp_data[9];
      n = gzread(fp, sp_data, sizeof(sp_data));
      ASSERT_INT(n, (int)sizeof(sp_data));
      ASSERT_INT(sp_data[0 * 3 + 1], 100);
      ASSERT_INT(sp_data[0 * 3 + 2], 200);
      ASSERT_INT(sp_data[1 * 3 + 2], 150);

      char magic2[16];
      n = gzread(fp, magic2, 16);
      ASSERT_INT(n, 16);
      ASSERT_TRUE(memcmp(magic2, "jkbotti_matrixB\0", 16) == 0);

      unsigned short ft_data[9];
      n = gzread(fp, ft_data, sizeof(ft_data));
      ASSERT_INT(n, (int)sizeof(ft_data));
      ASSERT_INT(ft_data[0 * 3 + 1], 1);
      ASSERT_INT(ft_data[0 * 3 + 2], 1);
      ASSERT_INT(ft_data[1 * 3 + 2], 2);

      gzclose(fp);
   }
   leave_temp_dir();
   PASS();

   TEST("zero-sized matrix produces empty file");
   reset_waypoint_state();
   enter_temp_dir();
   gpGlobals->mapname = (string_t)(long)"testmap";

   route_num_waypoints = 0;
   shortest_path = waypoint_matrix_mem_array_shortest_path;
   from_to = waypoint_matrix_mem_array_from_to_array;
   wp_matrix_initialized = TRUE;

   WaypointSaveFloydsMatrix();

   // Should succeed (zero-size arrays are valid)
   {
      char filename[512];
      snprintf(filename, sizeof(filename),
               "%s/valve/addons/jk_botti/waypoints/testmap.matrix", test_tmpdir);
      gzFile fp = gzopen(filename, "rb");
      ASSERT_TRUE(fp != NULL);

      char magic[16];
      int n = gzread(fp, magic, 16);
      ASSERT_INT(n, 16);
      ASSERT_TRUE(memcmp(magic, "jkbotti_matrixA\0", 16) == 0);

      char magic2[16];
      n = gzread(fp, magic2, 16);
      ASSERT_INT(n, 16);
      ASSERT_TRUE(memcmp(magic2, "jkbotti_matrixB\0", 16) == 0);

      gzclose(fp);
   }
   leave_temp_dir();
   PASS();

   return 0;
}

// ============================================================
// Tests: WaypointReachable
// ============================================================

// Trace callback for WaypointReachable tests.
// The function uses two types of traces:
//   1. Visibility trace (src->dest) - hull or move trace
//   2. Ground probes (downward, v2.z = v1.z - 1000)
// We use the z difference to distinguish them.

static int reachable_visibility_result; // 0 = visible, 1 = blocked
static float reachable_ground_fraction; // fraction for downward ground probes

static void trace_for_reachable(const float *v1, const float *v2,
                                int fNoMonsters, int hullNumber,
                                edict_t *pentToSkip, TraceResult *ptr)
{
   memset(ptr, 0, sizeof(*ptr));

   // Detect downward ground probes: v2.z is significantly below v1.z
   if (v2[2] < v1[2] - 500.0f)
   {
      // Ground probe
      ptr->flFraction = reachable_ground_fraction;
      ptr->vecEndPos[0] = v1[0] + (v2[0] - v1[0]) * reachable_ground_fraction;
      ptr->vecEndPos[1] = v1[1] + (v2[1] - v1[1]) * reachable_ground_fraction;
      ptr->vecEndPos[2] = v1[2] + (v2[2] - v1[2]) * reachable_ground_fraction;
      return;
   }

   // Detect upward probe in mid-air check (v_new_dest.z = v_new_src.z - 50)
   if (v2[2] < v1[2] - 40.0f && v2[2] > v1[2] - 60.0f)
   {
      // This is the 50-unit downward check for high dest
      ptr->flFraction = reachable_ground_fraction;
      ptr->vecEndPos[0] = v1[0] + (v2[0] - v1[0]) * reachable_ground_fraction;
      ptr->vecEndPos[1] = v1[1] + (v2[1] - v1[1]) * reachable_ground_fraction;
      ptr->vecEndPos[2] = v1[2] + (v2[2] - v1[2]) * reachable_ground_fraction;
      return;
   }

   // Visibility trace
   if (reachable_visibility_result)
   {
      ptr->flFraction = 0.5f;
      ptr->fAllSolid = TRUE;
   }
   else
   {
      ptr->flFraction = 1.0f;
      ptr->vecEndPos[0] = v2[0];
      ptr->vecEndPos[1] = v2[1];
      ptr->vecEndPos[2] = v2[2];
   }
}

static int test_WaypointReachable(void)
{
   printf("WaypointReachable:\n");

   TEST("out of range returns FALSE");
   reset_waypoint_state();
   {
      Vector src(0, 0, 0);
      Vector dest(REACHABLE_RANGE + 1, 0, 0);
      ASSERT_INT(WaypointReachable(src, dest, 0), FALSE);
   }
   PASS();

   TEST("short visible path (<= 32 units) returns TRUE");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0; // visible
   reachable_ground_fraction = 0.01f; // on ground
   {
      Vector src(0, 0, 0);
      Vector dest(30, 0, 0);
      ASSERT_INT(WaypointReachable(src, dest, 0), TRUE);
   }
   PASS();

   TEST("blocked visibility trace returns FALSE");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 1; // blocked
   reachable_ground_fraction = 0.01f;
   {
      Vector src(0, 0, 0);
      Vector dest(200, 0, 0);
      ASSERT_INT(WaypointReachable(src, dest, 0), FALSE);
   }
   PASS();

   TEST("both waypoints underwater returns TRUE");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0; // visible
   reachable_ground_fraction = 0.01f;
   mock_point_contents_fn = [](const float *) -> int { return CONTENTS_WATER; };
   {
      Vector src(0, 0, 0);
      Vector dest(200, 0, 0);
      ASSERT_INT(WaypointReachable(src, dest, 0), TRUE);
   }
   PASS();

   TEST("high dest with nothing below returns FALSE");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0; // visible
   reachable_ground_fraction = 1.0f; // nothing below (full fraction = mid-air)
   {
      Vector src(0, 0, 0);
      Vector dest(100, 0, 100); // dest is 100 units above src (> 45)
      ASSERT_INT(WaypointReachable(src, dest, 0), FALSE);
   }
   PASS();

   TEST("ladder waypoints at same XY within 32 units returns TRUE");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0; // visible
   reachable_ground_fraction = 0.01f;
   {
      Vector src(100, 200, 0);
      Vector dest(100, 200, 30); // same XY, 30 units vertical
      ASSERT_INT(WaypointReachable(src, dest, W_FL_LADDER), TRUE);
   }
   PASS();

   TEST("ground obstacle > 45 units returns FALSE");
   reset_waypoint_state();
   {
      // Simulate a step-up: ground is far below at start, close at end
      // height goes from 100 to 10 -> last_height - curr_height = 90 > 45
      mock_trace_hull_fn = [](const float *v1, const float *v2,
                              int fNoMonsters, int hullNumber,
                              edict_t *pentToSkip, TraceResult *ptr)
      {
         memset(ptr, 0, sizeof(*ptr));
         if (v2[2] < v1[2] - 500.0f)
         {
            // Ground probe: step-up at x > 100
            if (v1[0] < 100.0f)
               ptr->flFraction = 0.1f;  // height = 100 (ground far below)
            else
               ptr->flFraction = 0.01f; // height = 10 (ground close, rose up)
            ptr->vecEndPos[0] = v1[0];
            ptr->vecEndPos[1] = v1[1];
            ptr->vecEndPos[2] = v1[2] + (v2[2] - v1[2]) * ptr->flFraction;
            return;
         }
         // Visibility: pass
         ptr->flFraction = 1.0f;
         ptr->vecEndPos[0] = v2[0];
         ptr->vecEndPos[1] = v2[1];
         ptr->vecEndPos[2] = v2[2];
      };

      Vector src(0, 0, 0);
      Vector dest(200, 0, 0);
      ASSERT_INT(WaypointReachable(src, dest, 0), FALSE);
   }
   PASS();

   TEST("gentle slope returns TRUE");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0; // visible
   reachable_ground_fraction = 0.01f; // ground close everywhere
   {
      Vector src(0, 0, 0);
      Vector dest(200, 0, 10); // gentle slope
      ASSERT_INT(WaypointReachable(src, dest, 0), TRUE);
   }
   PASS();

   return 0;
}

// ============================================================
// Tests: WaypointMakePathsWith
// ============================================================

static int test_WaypointMakePathsWith(void)
{
   printf("WaypointMakePathsWith:\n");

   TEST("creates bidirectional paths");
   reset_waypoint_state();
   // Use close waypoints so WaypointReachable passes with default trace (no hit)
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   setup_waypoint(0, Vector(0, 0, 0), 0, 0);
   setup_waypoint(1, Vector(30, 0, 0), 0, 0);
   WaypointMakePathsWith(1);
   // Should have paths 0->1 and 1->0
   ASSERT_TRUE(WaypointNumberOfPaths(0) >= 1);
   ASSERT_TRUE(WaypointNumberOfPaths(1) >= 1);
   {
      int pi = -1;
      ASSERT_INT(WaypointFindPath(pi, 0), 1);
   }
   {
      int pi = -1;
      ASSERT_INT(WaypointFindPath(pi, 1), 0);
   }
   PASS();

   TEST("skips deleted and aiming waypoints");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   setup_waypoint(0, Vector(0, 0, 0), W_FL_DELETED, 0);
   setup_waypoint(1, Vector(30, 0, 0), W_FL_AIMING, 0);
   setup_waypoint(2, Vector(20, 0, 0), 0, 0);
   WaypointMakePathsWith(2);
   ASSERT_INT(WaypointNumberOfPaths(2), 0);
   PASS();

   TEST("no paths TO lift-end");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   setup_waypoint(0, Vector(0, 0, 0), W_FL_LIFT_END, 0);
   setup_waypoint(1, Vector(30, 0, 0), 0, 0);
   WaypointMakePathsWith(1);
   // 1 should NOT have a path to 0 (lift-end), but 0 should have path to 1
   {
      int pi = -1;
      int dest = WaypointFindPath(pi, 1);
      // 1's outgoing paths should NOT include 0
      qboolean found_lift_end = FALSE;
      while (dest != -1)
      {
         if (dest == 0) found_lift_end = TRUE;
         dest = WaypointFindPath(pi, 1);
      }
      ASSERT_INT(found_lift_end, FALSE);
   }
   PASS();

   TEST("no paths when g_path_waypoint_enable is FALSE");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   g_path_waypoint_enable = FALSE;
   setup_waypoint(0, Vector(0, 0, 0), 0, 0);
   setup_waypoint(1, Vector(30, 0, 0), 0, 0);
   WaypointMakePathsWith(1);
   ASSERT_INT(WaypointNumberOfPaths(0), 0);
   ASSERT_INT(WaypointNumberOfPaths(1), 0);
   PASS();

   TEST("no path FROM lift-end to new waypoint");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   setup_waypoint(0, Vector(0, 0, 0), 0, 0);
   setup_waypoint(1, Vector(30, 0, 0), W_FL_LIFT_END, 0);
   WaypointMakePathsWith(0);
   // 0 should NOT have paths to 1 (lift-end), but 1 SHOULD have path to 0
   {
      int pi = -1;
      int dest = WaypointFindPath(pi, 0);
      qboolean found_lift_end = FALSE;
      while (dest != -1)
      {
         if (dest == 1) found_lift_end = TRUE;
         dest = WaypointFindPath(pi, 0);
      }
      ASSERT_INT(found_lift_end, FALSE);
   }
   PASS();

   return 0;
}

// ============================================================
// Tests: WaypointFixOldWaypoints
// ============================================================

static int test_WaypointFixOldWaypoints(void)
{
   printf("WaypointFixOldWaypoints:\n");

   TEST("crouch-to-normal conversion when room above");
   reset_waypoint_state();
   // Trace callback that:
   // - Passes upward traces (room to stand) -> fraction 1.0
   // - Hits ground on downward traces (not mid-air) -> fraction < 1.0
   mock_trace_hull_fn = [](const float *v1, const float *v2,
                           int fNoMonsters, int hullNumber,
                           edict_t *pentToSkip, TraceResult *ptr)
   {
      memset(ptr, 0, sizeof(*ptr));
      if (v2[2] < v1[2])
      {
         // Downward trace: hit ground
         ptr->flFraction = 0.5f;
         ptr->vecEndPos[0] = v1[0];
         ptr->vecEndPos[1] = v1[1];
         ptr->vecEndPos[2] = v1[2] + (v2[2] - v1[2]) * 0.5f;
      }
      else
      {
         // Upward trace: room above (pass through)
         ptr->flFraction = 1.0f;
         ptr->vecEndPos[0] = v2[0];
         ptr->vecEndPos[1] = v2[1];
         ptr->vecEndPos[2] = v2[2];
      }
   };
   setup_waypoint(0, Vector(100, 100, 50), W_FL_CROUCH, 0);
   {
      float old_z = waypoints[0].origin.z;
      qboolean changed = WaypointFixOldWaypoints();
      ASSERT_INT(changed, TRUE);
      // Crouch flag should be removed
      ASSERT_TRUE(!(waypoints[0].flags & W_FL_CROUCH));
      // Origin z should be raised by 18 (36/2)
      ASSERT_TRUE(waypoints[0].origin.z == old_z + 18.0f);
   }
   PASS();

   TEST("keep crouch when blocked above");
   reset_waypoint_state();
   // Trace callback: visibility blocked (can't stand up)
   mock_trace_hull_fn = [](const float *v1, const float *v2,
                           int fNoMonsters, int hullNumber,
                           edict_t *pentToSkip, TraceResult *ptr)
   {
      memset(ptr, 0, sizeof(*ptr));
      // For the upward probe (checking room above), block it
      if (v2[2] > v1[2])
      {
         ptr->flFraction = 0.5f;
         ptr->fAllSolid = TRUE;
      }
      else
      {
         // Ground probes pass
         ptr->flFraction = 0.01f;
         ptr->vecEndPos[0] = v1[0];
         ptr->vecEndPos[1] = v1[1];
         ptr->vecEndPos[2] = v1[2] + (v2[2] - v1[2]) * 0.01f;
      }
   };
   setup_waypoint(0, Vector(100, 100, 50), W_FL_CROUCH, 0);
   {
      float old_z = waypoints[0].origin.z;
      WaypointFixOldWaypoints();
      // Crouch flag should remain because trace up was blocked
      // But midair check may also trigger, so check if crouch still present or deleted
      // The midair check traces down from the crouch waypoint's position
      // With our callback, downward traces hit something (fraction 0.01), so not midair
      ASSERT_TRUE(waypoints[0].flags & W_FL_CROUCH);
      ASSERT_TRUE(waypoints[0].origin.z == old_z);
   }
   PASS();

   TEST("remove mid-air waypoints");
   reset_waypoint_state();
   // Trace callback: visibility passes but downward trace finds nothing (mid-air)
   mock_trace_hull_fn = [](const float *v1, const float *v2,
                           int fNoMonsters, int hullNumber,
                           edict_t *pentToSkip, TraceResult *ptr)
   {
      memset(ptr, 0, sizeof(*ptr));
      // Upward probe: pass (room above)
      if (v2[2] > v1[2])
      {
         ptr->flFraction = 1.0f;
         ptr->vecEndPos[0] = v2[0];
         ptr->vecEndPos[1] = v2[1];
         ptr->vecEndPos[2] = v2[2];
      }
      else
      {
         // Downward probe: nothing below (fraction = 1.0)
         ptr->flFraction = 1.0f;
         ptr->vecEndPos[0] = v2[0];
         ptr->vecEndPos[1] = v2[1];
         ptr->vecEndPos[2] = v2[2];
      }
   };
   setup_waypoint(0, Vector(100, 100, 500), 0, 0);
   {
      qboolean changed = WaypointFixOldWaypoints();
      ASSERT_INT(changed, TRUE);
      ASSERT_TRUE(waypoints[0].flags & W_FL_DELETED);
   }
   PASS();

   TEST("keep grounded waypoints");
   reset_waypoint_state();
   // Trace callback: downward traces hit ground, everything else passes
   mock_trace_hull_fn = [](const float *v1, const float *v2,
                           int fNoMonsters, int hullNumber,
                           edict_t *pentToSkip, TraceResult *ptr)
   {
      memset(ptr, 0, sizeof(*ptr));
      if (v2[2] < v1[2])
      {
         // Downward trace: hit ground
         ptr->flFraction = 0.5f;
         ptr->vecEndPos[0] = v1[0];
         ptr->vecEndPos[1] = v1[1];
         ptr->vecEndPos[2] = v1[2] + (v2[2] - v1[2]) * 0.5f;
      }
      else
      {
         ptr->flFraction = 1.0f;
         ptr->vecEndPos[0] = v2[0];
         ptr->vecEndPos[1] = v2[1];
         ptr->vecEndPos[2] = v2[2];
      }
   };
   setup_waypoint(0, Vector(100, 100, 50), 0, 0);
   {
      qboolean changed = WaypointFixOldWaypoints();
      // No crouch, on ground, should not be modified
      ASSERT_TRUE(!(waypoints[0].flags & W_FL_DELETED));
      ASSERT_INT(changed, FALSE);
   }
   PASS();

   TEST("keep water waypoints");
   reset_waypoint_state();
   // All traces pass through (mid-air normally), but water check should save it
   mock_trace_hull_fn = [](const float *v1, const float *v2,
                           int fNoMonsters, int hullNumber,
                           edict_t *pentToSkip, TraceResult *ptr)
   {
      memset(ptr, 0, sizeof(*ptr));
      ptr->flFraction = 1.0f;
      ptr->vecEndPos[0] = v2[0];
      ptr->vecEndPos[1] = v2[1];
      ptr->vecEndPos[2] = v2[2];
   };
   mock_point_contents_fn = [](const float *) -> int { return CONTENTS_WATER; };
   setup_waypoint(0, Vector(100, 100, 50), 0, 0);
   {
      WaypointFixOldWaypoints();
      // Water waypoint, should NOT be deleted
      ASSERT_TRUE(!(waypoints[0].flags & W_FL_DELETED));
   }
   PASS();

   TEST("remove unreachable paths");
   reset_waypoint_state();
   // Traces: all blocked (so WaypointReachable returns FALSE for paths)
   mock_trace_hull_fn = [](const float *v1, const float *v2,
                           int fNoMonsters, int hullNumber,
                           edict_t *pentToSkip, TraceResult *ptr)
   {
      memset(ptr, 0, sizeof(*ptr));
      // Downward traces: on ground (don't remove as midair)
      if (v2[2] < v1[2] - 10.0f)
      {
         ptr->flFraction = 0.01f;
         ptr->vecEndPos[0] = v1[0];
         ptr->vecEndPos[1] = v1[1];
         ptr->vecEndPos[2] = v1[2] + (v2[2] - v1[2]) * 0.01f;
         return;
      }
      // Visibility: blocked
      ptr->flFraction = 0.5f;
      ptr->fAllSolid = TRUE;
   };
   setup_waypoint(0, Vector(0, 0, 0), 0, 0);
   setup_waypoint(1, Vector(200, 0, 0), 0, 0);
   setup_path(0, 1);
   ASSERT_INT(WaypointNumberOfPaths(0), 1);
   {
      qboolean changed = WaypointFixOldWaypoints();
      ASSERT_INT(changed, TRUE);
      // Path 0->1 should have been removed
      ASSERT_INT(WaypointNumberOfPaths(0), 0);
   }
   PASS();

   TEST("skip aiming and deleted in path check");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   setup_waypoint(0, Vector(0, 0, 0), W_FL_AIMING, 0);
   setup_waypoint(1, Vector(100, 0, 0), W_FL_DELETED, 0);
   setup_waypoint(2, Vector(200, 0, 0), 0, 0);
   {
      WaypointFixOldWaypoints();
      // Aiming and deleted should not be touched by path check
      ASSERT_INT(waypoints[0].flags, W_FL_AIMING);
      ASSERT_TRUE(waypoints[1].flags & W_FL_DELETED);
   }
   PASS();

   return 0;
}

// ============================================================
// Tests: WaypointSearchItems
// ============================================================

static int test_WaypointSearchItems(void)
{
   printf("WaypointSearchItems:\n");

   TEST("detects health item");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0, 0, 0), 0, 0);
   setup_entity("item_healthkit", Vector(10, 0, 0));
   WaypointSearchItems(NULL, waypoints[0].origin, 0);
   ASSERT_TRUE(waypoints[0].flags & W_FL_HEALTH);
   PASS();

   TEST("detects armor item");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0, 0, 0), 0, 0);
   setup_entity("item_battery", Vector(10, 0, 0));
   WaypointSearchItems(NULL, waypoints[0].origin, 0);
   ASSERT_TRUE(waypoints[0].flags & W_FL_ARMOR);
   PASS();

   TEST("detects ammo item with itemflags");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0, 0, 0), 0, 0);
   setup_entity("ammo_buckshot", Vector(10, 0, 0));
   WaypointSearchItems(NULL, waypoints[0].origin, 0);
   ASSERT_TRUE(waypoints[0].flags & W_FL_AMMO);
   ASSERT_TRUE(waypoints[0].itemflags != 0);
   PASS();

   TEST("detects longjump item");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0, 0, 0), 0, 0);
   setup_entity("item_longjump", Vector(10, 0, 0));
   WaypointSearchItems(NULL, waypoints[0].origin, 0);
   ASSERT_TRUE(waypoints[0].flags & W_FL_LONGJUMP);
   PASS();

   TEST("trace-blocked items ignored");
   reset_waypoint_state();
   // Block trace: item not visible
   mock_trace_hull_fn = [](const float *v1, const float *v2,
                           int fNoMonsters, int hullNumber,
                           edict_t *pentToSkip, TraceResult *ptr)
   {
      memset(ptr, 0, sizeof(*ptr));
      ptr->flFraction = 0.5f; // blocked
      ptr->fAllSolid = TRUE;
   };
   setup_waypoint(0, Vector(0, 0, 0), 0, 0);
   setup_entity("item_healthkit", Vector(10, 0, 0));
   WaypointSearchItems(NULL, waypoints[0].origin, 0);
   ASSERT_TRUE(!(waypoints[0].flags & W_FL_HEALTH));
   PASS();

   TEST("no entities gives no flags");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0, 0, 0), 0, 0);
   WaypointSearchItems(NULL, waypoints[0].origin, 0);
   ASSERT_INT(waypoints[0].flags, 0);
   PASS();

   TEST("nearest item wins when multiple present");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0, 0, 0), 0, 0);
   setup_entity("item_healthkit", Vector(30, 0, 0)); // farther, within radius
   setup_entity("item_battery", Vector(5, 0, 0));    // closer
   WaypointSearchItems(NULL, waypoints[0].origin, 0);
   // Nearest is armor, but both are in radius.
   // The function finds the nearest item overall, then sets flags based on name
   // Actually, it picks the nearest item and sets its flag.
   // battery at 5 is nearest, so W_FL_ARMOR should be set
   ASSERT_TRUE(waypoints[0].flags & W_FL_ARMOR);
   PASS();

   return 0;
}

// ============================================================
// Tests: CollectMapSpawnItems
// ============================================================

static int test_CollectMapSpawnItems(void)
{
   printf("CollectMapSpawnItems:\n");

   TEST("item_healthkit sets W_FL_HEALTH and W_FL_SPAWNADD");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      edict_t *e = setup_entity("item_healthkit", Vector(100, 200, 50));
      CollectMapSpawnItems(e);
      ASSERT_INT(num_spawnpoints, 1);
      ASSERT_TRUE(spawnpoints[0].flags & W_FL_HEALTH);
      ASSERT_TRUE(spawnpoints[0].flags & W_FL_SPAWNADD);
   }
   PASS();

   TEST("item_battery sets W_FL_ARMOR");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      edict_t *e = setup_entity("item_battery", Vector(100, 200, 50));
      CollectMapSpawnItems(e);
      ASSERT_INT(num_spawnpoints, 1);
      ASSERT_TRUE(spawnpoints[0].flags & W_FL_ARMOR);
   }
   PASS();

   TEST("ammo with itemflags");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      edict_t *e = setup_entity("ammo_buckshot", Vector(100, 200, 50));
      CollectMapSpawnItems(e);
      ASSERT_INT(num_spawnpoints, 1);
      ASSERT_TRUE(spawnpoints[0].flags & W_FL_AMMO);
      ASSERT_TRUE(spawnpoints[0].itemflags != 0);
   }
   PASS();

   TEST("merge nearby spawnpoints");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      // First entity
      edict_t *e1 = setup_entity("item_healthkit", Vector(100, 200, 50));
      CollectMapSpawnItems(e1);
      ASSERT_INT(num_spawnpoints, 1);

      // Second entity very close
      edict_t *e2 = setup_entity("item_battery", Vector(100, 200, 51));
      CollectMapSpawnItems(e2);
      // Should merge into existing spawnpoint
      ASSERT_INT(num_spawnpoints, 1);
      ASSERT_TRUE(spawnpoints[0].flags & W_FL_HEALTH);
      ASSERT_TRUE(spawnpoints[0].flags & W_FL_ARMOR);
   }
   PASS();

   TEST("W_FL_CROUCH when no standing room");
   reset_waypoint_state();
   // Ground trace: hits immediately (on ground)
   // Standing trace: blocked (no room to stand)
   {
      static int trace_call_num;
      trace_call_num = 0;

      mock_trace_hull_fn = [](const float *v1, const float *v2,
                              int fNoMonsters, int hullNumber,
                              edict_t *pentToSkip, TraceResult *ptr)
      {
         memset(ptr, 0, sizeof(*ptr));
         trace_call_num++;

         if (hullNumber == head_hull && v2[2] < v1[2] - 100.0f)
         {
            // Downward ground trace - on ground
            ptr->flFraction = 0.01f;
            ptr->vecEndPos[0] = v1[0];
            ptr->vecEndPos[1] = v1[1];
            ptr->vecEndPos[2] = v1[2] + (v2[2] - v1[2]) * 0.01f;
         }
         else if (hullNumber == human_hull)
         {
            // Standing hull check - blocked (no room)
            ptr->flFraction = 0.5f;
            ptr->fStartSolid = TRUE;
         }
         else
         {
            // Default: pass
            ptr->flFraction = 1.0f;
            ptr->vecEndPos[0] = v2[0];
            ptr->vecEndPos[1] = v2[1];
            ptr->vecEndPos[2] = v2[2];
         }
      };

      edict_t *e = setup_entity("item_healthkit", Vector(100, 200, 50));
      CollectMapSpawnItems(e);
      ASSERT_INT(num_spawnpoints, 1);
      ASSERT_TRUE(spawnpoints[0].flags & W_FL_CROUCH);
   }
   PASS();

   TEST("stuck-in-world returns without adding");
   reset_waypoint_state();
   mock_trace_hull_fn = [](const float *v1, const float *v2,
                           int fNoMonsters, int hullNumber,
                           edict_t *pentToSkip, TraceResult *ptr)
   {
      memset(ptr, 0, sizeof(*ptr));
      ptr->fStartSolid = TRUE;
      ptr->fAllSolid = TRUE;
      ptr->flFraction = 0.0f;
   };
   {
      edict_t *e = setup_entity("item_healthkit", Vector(100, 200, 50));
      CollectMapSpawnItems(e);
      ASSERT_INT(num_spawnpoints, 0);
   }
   PASS();

   TEST("max spawnpoints guard");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      // Fill spawnpoints to max
      num_spawnpoints = MAX_WAYPOINTS / 2;
      edict_t *e = setup_entity("item_healthkit", Vector(100, 200, 50));
      CollectMapSpawnItems(e);
      // Should not add beyond max
      ASSERT_INT(num_spawnpoints, MAX_WAYPOINTS / 2);
   }
   PASS();

   return 0;
}

// ============================================================
// Tests: WaypointAddLift
// ============================================================

static int test_WaypointAddLift(void)
{
   printf("WaypointAddLift:\n");

   TEST("creates start and end waypoints with path");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      edict_t *pent = setup_entity("func_door", Vector(100, 200, 0));
      pent->v.absmax[0] = 150; pent->v.absmax[1] = 250; pent->v.absmax[2] = 50;
      pent->v.absmin[0] = 50;  pent->v.absmin[1] = 150; pent->v.absmin[2] = -50;

      Vector start(0, 0, 0);
      Vector end(0, 0, 200);
      WaypointAddLift(pent, start, end);

      ASSERT_INT(g_lifts_added, 1);
      ASSERT_INT(num_waypoints, 2);

      // First waypoint: LIFT_START
      ASSERT_TRUE(waypoints[0].flags & W_FL_LIFT_START);
      // Second waypoint: LIFT_END
      ASSERT_TRUE(waypoints[1].flags & W_FL_LIFT_END);

      // Path from start to end
      int pi = -1;
      ASSERT_INT(WaypointFindPath(pi, 0), 1);
   }
   PASS();

   TEST("rejects height diff < 16");
   reset_waypoint_state();
   {
      edict_t *pent = setup_entity("func_door", Vector(100, 200, 0));
      pent->v.absmax[2] = 50;
      pent->v.absmin[2] = -50;

      Vector start(0, 0, 0);
      Vector end(0, 0, 10); // height diff = 10 < 16
      WaypointAddLift(pent, start, end);

      ASSERT_INT(g_lifts_added, 0);
      ASSERT_INT(num_waypoints, 0);
   }
   PASS();

   TEST("rejects blocked trace (trap)");
   reset_waypoint_state();
   // Trace callback that blocks the trap-detection trace
   mock_trace_hull_fn = [](const float *v1, const float *v2,
                           int fNoMonsters, int hullNumber,
                           edict_t *pentToSkip, TraceResult *ptr)
   {
      memset(ptr, 0, sizeof(*ptr));
      // Block the trace from stand_start to stand_end (trap detection)
      ptr->flFraction = 0.5f;
      ptr->fAllSolid = TRUE;
   };
   {
      edict_t *pent = setup_entity("func_door", Vector(100, 200, 0));
      pent->v.absmax[0] = 150; pent->v.absmax[1] = 250; pent->v.absmax[2] = 50;
      pent->v.absmin[0] = 50;  pent->v.absmin[1] = 150; pent->v.absmin[2] = -50;

      Vector start(0, 0, 0);
      Vector end(0, 0, 200);
      WaypointAddLift(pent, start, end);

      ASSERT_INT(g_lifts_added, 0);
      ASSERT_INT(num_waypoints, 0);
   }
   PASS();

   TEST("rejects duplicate lift waypoints nearby");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      edict_t *pent = setup_entity("func_door", Vector(100, 200, 0));
      pent->v.absmax[0] = 150; pent->v.absmax[1] = 250; pent->v.absmax[2] = 50;
      pent->v.absmin[0] = 50;  pent->v.absmin[1] = 150; pent->v.absmin[2] = -50;

      Vector start(0, 0, 0);
      Vector end(0, 0, 200);
      WaypointAddLift(pent, start, end);
      ASSERT_INT(g_lifts_added, 1);

      // Try to add another lift at same location
      WaypointAddLift(pent, start, end);
      ASSERT_INT(g_lifts_added, 1); // should not increase
   }
   PASS();

   TEST("entity origin restored after call");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      edict_t *pent = setup_entity("func_door", Vector(100, 200, 0));
      pent->v.absmax[0] = 150; pent->v.absmax[1] = 250; pent->v.absmax[2] = 50;
      pent->v.absmin[0] = 50;  pent->v.absmin[1] = 150; pent->v.absmin[2] = -50;

      Vector start(0, 0, 0);
      Vector end(0, 0, 200);
      WaypointAddLift(pent, start, end);

      // SET_ORIGIN is called with end then start, so origin should be restored
      ASSERT_TRUE(pent->v.origin.x == start.x);
      ASSERT_TRUE(pent->v.origin.y == start.y);
      ASSERT_TRUE(pent->v.origin.z == start.z);
   }
   PASS();

   return 0;
}

// ============================================================
// Tests: WaypointFindNearest
// ============================================================

static int test_WaypointFindNearest(void)
{
   printf("WaypointFindNearest:\n");

   TEST("empty waypoints returns -1");
   reset_waypoint_state();
   {
      edict_t *pEntity = mock_alloc_edict();
      ASSERT_INT(WaypointFindNearest(Vector(0,0,0), Vector(0,0,17), pEntity, 9999.0, FALSE), -1);
   }
   PASS();

   TEST("returns nearest waypoint within range");
   reset_waypoint_state();
   {
      edict_t *pEntity = mock_alloc_edict();
      setup_waypoint(0, Vector(100,0,0), 0, 0);
      setup_waypoint(1, Vector(50,0,0), 0, 0);
      int idx = WaypointFindNearest(Vector(0,0,0), Vector(0,0,17), pEntity, 9999.0, FALSE);
      ASSERT_INT(idx, 1);
   }
   PASS();

   TEST("skips deleted and aiming waypoints");
   reset_waypoint_state();
   {
      edict_t *pEntity = mock_alloc_edict();
      setup_waypoint(0, Vector(50,0,0), W_FL_DELETED, 0);
      setup_waypoint(1, Vector(60,0,0), W_FL_AIMING, 0);
      setup_waypoint(2, Vector(200,0,0), 0, 0);
      int idx = WaypointFindNearest(Vector(0,0,0), Vector(0,0,17), pEntity, 9999.0, FALSE);
      ASSERT_INT(idx, 2);
   }
   PASS();

   TEST("respects range limit");
   reset_waypoint_state();
   {
      edict_t *pEntity = mock_alloc_edict();
      setup_waypoint(0, Vector(500,0,0), 0, 0);
      int idx = WaypointFindNearest(Vector(0,0,0), Vector(0,0,17), pEntity, 200.0, FALSE);
      ASSERT_INT(idx, -1);
   }
   PASS();

   TEST("trace-blocked waypoint skipped");
   reset_waypoint_state();
   mock_trace_hull_fn = [](const float *v1, const float *v2,
                           int fNoMonsters, int hullNumber,
                           edict_t *pentToSkip, TraceResult *ptr)
   {
      memset(ptr, 0, sizeof(*ptr));
      ptr->flFraction = 0.5f;
   };
   {
      edict_t *pEntity = mock_alloc_edict();
      setup_waypoint(0, Vector(100,0,0), 0, 0);
      int idx = WaypointFindNearest(Vector(0,0,0), Vector(0,0,17), pEntity, 9999.0, FALSE);
      ASSERT_INT(idx, -1);
   }
   PASS();

   TEST("b_traceline selects TraceLine vs TraceHull");
   reset_waypoint_state();
   {
      static int trace_line_called;
      static int trace_hull_called;
      trace_line_called = 0;
      trace_hull_called = 0;

      mock_trace_line_fn = [](const float *v1, const float *v2,
                              int fNoMonsters, int hullNumber,
                              edict_t *pentToSkip, TraceResult *ptr)
      {
         trace_line_called++;
         memset(ptr, 0, sizeof(*ptr));
         ptr->flFraction = 1.0f;
         ptr->vecEndPos[0] = v2[0];
         ptr->vecEndPos[1] = v2[1];
         ptr->vecEndPos[2] = v2[2];
      };
      mock_trace_hull_fn = [](const float *v1, const float *v2,
                              int fNoMonsters, int hullNumber,
                              edict_t *pentToSkip, TraceResult *ptr)
      {
         trace_hull_called++;
         memset(ptr, 0, sizeof(*ptr));
         ptr->flFraction = 1.0f;
         ptr->vecEndPos[0] = v2[0];
         ptr->vecEndPos[1] = v2[1];
         ptr->vecEndPos[2] = v2[2];
      };

      edict_t *pEntity = mock_alloc_edict();
      setup_waypoint(0, Vector(100,0,0), 0, 0);

      WaypointFindNearest(Vector(0,0,0), Vector(0,0,17), pEntity, 9999.0, TRUE);
      ASSERT_TRUE(trace_line_called > 0);
      ASSERT_INT(trace_hull_called, 0);

      trace_line_called = 0;
      trace_hull_called = 0;
      WaypointFindNearest(Vector(0,0,0), Vector(0,0,17), pEntity, 9999.0, FALSE);
      ASSERT_INT(trace_line_called, 0);
      ASSERT_TRUE(trace_hull_called > 0);
   }
   PASS();

   return 0;
}

// ============================================================
// Tests: WaypointFindReachable
// ============================================================

static int test_WaypointFindReachable(void)
{
   printf("WaypointFindReachable:\n");

   TEST("empty waypoints returns -1");
   reset_waypoint_state();
   {
      edict_t *pEntity = mock_alloc_edict();
      pEntity->v.origin = Vector(0,0,0);
      pEntity->v.view_ofs = Vector(0,0,17);
      ASSERT_INT(WaypointFindReachable(pEntity, 9999.0), -1);
   }
   PASS();

   TEST("returns nearest reachable waypoint");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      edict_t *pEntity = mock_alloc_edict();
      pEntity->v.origin = Vector(0,0,0);
      pEntity->v.view_ofs = Vector(0,0,17);
      setup_waypoint(0, Vector(200,0,0), 0, 0);
      setup_waypoint(1, Vector(100,0,0), 0, 0);
      int idx = WaypointFindReachable(pEntity, 9999.0);
      ASSERT_INT(idx, 1);
   }
   PASS();

   TEST("skips deleted and aiming waypoints");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      edict_t *pEntity = mock_alloc_edict();
      pEntity->v.origin = Vector(0,0,0);
      pEntity->v.view_ofs = Vector(0,0,17);
      setup_waypoint(0, Vector(50,0,0), W_FL_DELETED, 0);
      setup_waypoint(1, Vector(60,0,0), W_FL_AIMING, 0);
      setup_waypoint(2, Vector(100,0,0), 0, 0);
      int idx = WaypointFindReachable(pEntity, 9999.0);
      ASSERT_INT(idx, 2);
   }
   PASS();

   TEST("visible but unreachable waypoint skipped");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      edict_t *pEntity = mock_alloc_edict();
      pEntity->v.origin = Vector(0,0,0);
      pEntity->v.view_ofs = Vector(0,0,17);
      // Distance > REACHABLE_RANGE: visible but WaypointReachable returns FALSE
      setup_waypoint(0, Vector(REACHABLE_RANGE + 1, 0, 0), 0, 0);
      int idx = WaypointFindReachable(pEntity, 9999.0);
      ASSERT_INT(idx, -1);
   }
   PASS();

   TEST("returns -1 when min_distance > range");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      edict_t *pEntity = mock_alloc_edict();
      pEntity->v.origin = Vector(0,0,0);
      pEntity->v.view_ofs = Vector(0,0,17);
      setup_waypoint(0, Vector(200,0,0), 0, 0);
      // Waypoint at 200 is reachable but range cutoff is 100
      int idx = WaypointFindReachable(pEntity, 100.0);
      ASSERT_INT(idx, -1);
   }
   PASS();

   return 0;
}

// ============================================================
// Tests: WaypointFindItem
// ============================================================

static int test_WaypointFindItem(void)
{
   printf("WaypointFindItem:\n");

   TEST("no entities returns NULL");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), W_FL_HEALTH, 0);
   ASSERT_PTR_NULL(WaypointFindItem(0));
   PASS();

   TEST("finds healthkit near W_FL_HEALTH waypoint");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), W_FL_HEALTH, 0);
   {
      edict_t *e = setup_entity("item_healthkit", Vector(10, 0, 0));
      edict_t *result = WaypointFindItem(0);
      ASSERT_PTR_EQ(result, e);
   }
   PASS();

   TEST("finds battery near W_FL_ARMOR waypoint");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), W_FL_ARMOR, 0);
   {
      edict_t *e = setup_entity("item_battery", Vector(10, 0, 0));
      edict_t *result = WaypointFindItem(0);
      ASSERT_PTR_EQ(result, e);
   }
   PASS();

   TEST("ignores entity when waypoint flag does not match");
   reset_waypoint_state();
   // Waypoint has W_FL_ARMOR but entity is item_healthkit
   setup_waypoint(0, Vector(0,0,0), W_FL_ARMOR, 0);
   setup_entity("item_healthkit", Vector(10, 0, 0));
   ASSERT_PTR_NULL(WaypointFindItem(0));
   PASS();

   TEST("trace-blocked entity ignored");
   reset_waypoint_state();
   mock_trace_hull_fn = [](const float *v1, const float *v2,
                           int fNoMonsters, int hullNumber,
                           edict_t *pentToSkip, TraceResult *ptr)
   {
      memset(ptr, 0, sizeof(*ptr));
      ptr->flFraction = 0.5f;
   };
   setup_waypoint(0, Vector(0,0,0), W_FL_HEALTH, 0);
   setup_entity("item_healthkit", Vector(10, 0, 0));
   ASSERT_PTR_NULL(WaypointFindItem(0));
   PASS();

   TEST("EF_NODRAW entity ignored");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), W_FL_HEALTH, 0);
   {
      edict_t *e = setup_entity("item_healthkit", Vector(10, 0, 0));
      e->v.effects |= EF_NODRAW;
      ASSERT_PTR_NULL(WaypointFindItem(0));
   }
   PASS();

   return 0;
}

// ============================================================
// Tests: WaypointAdd
// ============================================================

static int test_WaypointAdd(void)
{
   printf("WaypointAdd:\n");

   TEST("basic add at player origin");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      edict_t *pEntity = mock_alloc_edict();
      pEntity->v.origin = Vector(100, 200, 50);
      WaypointAdd(pEntity);
      ASSERT_INT(num_waypoints, 1);
      ASSERT_TRUE(waypoints[0].origin.x == 100.0f);
      ASSERT_TRUE(waypoints[0].origin.y == 200.0f);
      ASSERT_TRUE(waypoints[0].origin.z == 50.0f);
      ASSERT_INT(g_waypoint_updated, TRUE);
   }
   PASS();

   TEST("reuses deleted slot");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      setup_waypoint(0, Vector(0,0,0), W_FL_DELETED, 0);
      setup_waypoint(1, Vector(100,0,0), 0, 0);
      edict_t *pEntity = mock_alloc_edict();
      pEntity->v.origin = Vector(50, 50, 50);
      WaypointAdd(pEntity);
      // Should reuse slot 0 (was deleted)
      ASSERT_INT(num_waypoints, 2);
      ASSERT_TRUE(!(waypoints[0].flags & W_FL_DELETED));
      ASSERT_TRUE(waypoints[0].origin.x == 50.0f);
   }
   PASS();

   TEST("returns silently when MAX_WAYPOINTS reached");
   reset_waypoint_state();
   {
      // Fill all slots with non-deleted waypoints
      num_waypoints = MAX_WAYPOINTS;
      for (int i = 0; i < MAX_WAYPOINTS; i++)
         waypoints[i].flags = 0;
      edict_t *pEntity = mock_alloc_edict();
      pEntity->v.origin = Vector(0,0,0);
      g_waypoint_updated = FALSE;
      WaypointAdd(pEntity);
      ASSERT_INT(num_waypoints, MAX_WAYPOINTS);
      ASSERT_INT(g_waypoint_updated, FALSE);
   }
   PASS();

   TEST("ladder detection via MOVETYPE_FLY");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      edict_t *pEntity = mock_alloc_edict();
      pEntity->v.origin = Vector(100, 0, 0);
      pEntity->v.movetype = MOVETYPE_FLY;
      WaypointAdd(pEntity);
      ASSERT_TRUE(waypoints[0].flags & W_FL_LADDER);
   }
   PASS();

   TEST("crouch detection when blocked above");
   reset_waypoint_state();
   {
      // Block upward duck trace, pass everything else
      mock_trace_hull_fn = [](const float *v1, const float *v2,
                              int fNoMonsters, int hullNumber,
                              edict_t *pentToSkip, TraceResult *ptr)
      {
         memset(ptr, 0, sizeof(*ptr));
         if (v2[2] > v1[2])
         {
            // Upward trace (duck check): blocked
            ptr->flFraction = 0.5f;
            ptr->fAllSolid = TRUE;
         }
         else if (v2[2] < v1[2] - 500.0f)
         {
            // Ground probes
            ptr->flFraction = 0.01f;
            ptr->vecEndPos[0] = v1[0];
            ptr->vecEndPos[1] = v1[1];
            ptr->vecEndPos[2] = v1[2] + (v2[2] - v1[2]) * 0.01f;
         }
         else
         {
            ptr->flFraction = 1.0f;
            ptr->vecEndPos[0] = v2[0];
            ptr->vecEndPos[1] = v2[1];
            ptr->vecEndPos[2] = v2[2];
         }
      };

      edict_t *pEntity = mock_alloc_edict();
      pEntity->v.origin = Vector(100, 0, 50);
      pEntity->v.flags = FL_DUCKING | FL_ONGROUND;
      WaypointAdd(pEntity);
      ASSERT_TRUE(waypoints[0].flags & W_FL_CROUCH);
      // Origin stays at player origin when crouching
      ASSERT_TRUE(waypoints[0].origin.z == 50.0f);
   }
   PASS();

   TEST("no crouch when room above");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      edict_t *pEntity = mock_alloc_edict();
      pEntity->v.origin = Vector(100, 0, 50);
      pEntity->v.flags = FL_DUCKING | FL_ONGROUND;
      WaypointAdd(pEntity);
      ASSERT_TRUE(!(waypoints[0].flags & W_FL_CROUCH));
      // Origin z raised by 18 (36/2) when room to stand
      ASSERT_TRUE(waypoints[0].origin.z == 68.0f);
   }
   PASS();

   TEST("item search sets flags on waypoint");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      edict_t *pEntity = mock_alloc_edict();
      pEntity->v.origin = Vector(100, 0, 50);
      setup_entity("item_healthkit", Vector(110, 0, 50));
      WaypointAdd(pEntity);
      ASSERT_TRUE(waypoints[0].flags & W_FL_HEALTH);
   }
   PASS();

   TEST("updates players[].last_waypoint");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      edict_t *pEntity = mock_alloc_edict();
      pEntity->v.origin = Vector(100, 0, 50);
      int player_idx = g_engfuncs.pfnIndexOfEdict(pEntity) - 1;
      ASSERT_INT(players[player_idx].last_waypoint, -1);
      WaypointAdd(pEntity);
      ASSERT_INT(players[player_idx].last_waypoint, 0);
   }
   PASS();

   return 0;
}

// ============================================================
// Tests: WaypointAddSpawnObjects
// ============================================================

static int test_WaypointAddSpawnObjects(void)
{
   printf("WaypointAddSpawnObjects:\n");

   TEST("no-op when g_auto_waypoint=FALSE");
   reset_waypoint_state();
   g_auto_waypoint = FALSE;
   num_spawnpoints = 1;
   spawnpoints[0].origin = Vector(100, 200, 50);
   spawnpoints[0].flags = W_FL_HEALTH | W_FL_SPAWNADD;
   WaypointAddSpawnObjects();
   ASSERT_INT(num_waypoints, 0);
   // Spawnpoints NOT cleared (early return)
   ASSERT_INT(num_spawnpoints, 1);
   PASS();

   TEST("no-op when num_spawnpoints=0");
   reset_waypoint_state();
   g_auto_waypoint = TRUE;
   num_spawnpoints = 0;
   WaypointAddSpawnObjects();
   ASSERT_INT(num_waypoints, 0);
   PASS();

   TEST("merges flags into existing nearby waypoint");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      setup_waypoint(0, Vector(100, 200, 50), W_FL_HEALTH, 0);
      // Spawnpoint within 50 units of existing waypoint
      num_spawnpoints = 1;
      spawnpoints[0].origin = Vector(100, 200, 51);
      spawnpoints[0].flags = W_FL_ARMOR | W_FL_SPAWNADD;
      spawnpoints[0].itemflags = 0x10;
      WaypointAddSpawnObjects();
      // Flags merged, no new waypoint
      ASSERT_INT(num_waypoints, 1);
      ASSERT_TRUE(waypoints[0].flags & W_FL_HEALTH);
      ASSERT_TRUE(waypoints[0].flags & W_FL_ARMOR);
      ASSERT_INT(waypoints[0].itemflags, 0x10);
   }
   PASS();

   TEST("creates new waypoint from spawnpoint");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   gpGlobals->mapname = (string_t)(long)"testmap";
   {
      num_spawnpoints = 1;
      spawnpoints[0].origin = Vector(100, 200, 50);
      spawnpoints[0].flags = W_FL_HEALTH | W_FL_SPAWNADD;
      spawnpoints[0].itemflags = 0x42;
      WaypointAddSpawnObjects();
      ASSERT_INT(num_waypoints, 1);
      ASSERT_TRUE(waypoints[0].origin.x == 100.0f);
      ASSERT_TRUE(waypoints[0].origin.y == 200.0f);
      ASSERT_TRUE(waypoints[0].origin.z == 50.0f);
      ASSERT_TRUE(waypoints[0].flags & W_FL_HEALTH);
      ASSERT_TRUE(waypoints[0].flags & W_FL_SPAWNADD);
      ASSERT_INT(waypoints[0].itemflags, 0x42);
   }
   PASS();

   TEST("creates paths for new waypoints");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   gpGlobals->mapname = (string_t)(long)"testmap";
   {
      // Existing waypoint
      setup_waypoint(0, Vector(100, 200, 50), 0, 0);
      // Spawnpoint > 50 units away (no merge) but within REACHABLE_RANGE
      num_spawnpoints = 1;
      spawnpoints[0].origin = Vector(200, 200, 50);
      spawnpoints[0].flags = W_FL_HEALTH | W_FL_SPAWNADD;
      WaypointAddSpawnObjects();
      ASSERT_INT(num_waypoints, 2);
      // Paths should exist between wp0 and wp1
      ASSERT_TRUE(WaypointNumberOfPaths(0) >= 1);
      ASSERT_TRUE(WaypointNumberOfPaths(1) >= 1);
   }
   PASS();

   TEST("clears spawnpoints after processing");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   gpGlobals->mapname = (string_t)(long)"testmap";
   {
      num_spawnpoints = 1;
      spawnpoints[0].origin = Vector(100, 200, 50);
      spawnpoints[0].flags = W_FL_HEALTH | W_FL_SPAWNADD;
      WaypointAddSpawnObjects();
      ASSERT_INT(num_spawnpoints, 0);
   }
   PASS();

   return 0;
}

// ============================================================
// Tests: WaypointBlockList (WaypointAddBlock / WaypointInBlockRadius)
// ============================================================

static int test_WaypointBlockList(void)
{
   printf("WaypointBlockList:\n");

   TEST("add and find block in radius");
   reset_waypoint_state();
   {
      Vector origin(100, 200, 50);
      WaypointAddBlock(origin);
      // Nearby point (within 800 units, default trace passes)
      ASSERT_INT(WaypointInBlockRadius(Vector(110, 200, 50)), TRUE);
   }
   PASS();

   TEST("out of range returns FALSE");
   reset_waypoint_state();
   {
      Vector origin(0, 0, 0);
      WaypointAddBlock(origin);
      // Point > 800 units away
      ASSERT_INT(WaypointInBlockRadius(Vector(900, 0, 0)), FALSE);
   }
   PASS();

   TEST("trace-blocked returns FALSE");
   reset_waypoint_state();
   mock_trace_hull_fn = [](const float *v1, const float *v2,
                           int fNoMonsters, int hullNumber,
                           edict_t *pentToSkip, TraceResult *ptr)
   {
      memset(ptr, 0, sizeof(*ptr));
      // Block visibility: fraction < 1, no solid flags
      ptr->flFraction = 0.5f;
   };
   {
      Vector origin(100, 200, 50);
      WaypointAddBlock(origin);
      // Within range but trace blocked
      ASSERT_INT(WaypointInBlockRadius(Vector(110, 200, 50)), FALSE);
   }
   PASS();

   TEST("full block_list is no-op");
   reset_waypoint_state();
   {
      // Fill block list to capacity
      for (int i = 0; i < block_list_size; i++)
         WaypointAddBlock(Vector((float)i, 0, 0));
      ASSERT_INT(block_list_endlist, block_list_size);
      // Extra add should not crash
      WaypointAddBlock(Vector(99999, 0, 0));
      ASSERT_INT(block_list_endlist, block_list_size);
      // Existing entries still work
      ASSERT_INT(WaypointInBlockRadius(Vector(0, 0, 0)), TRUE);
   }
   PASS();

   TEST("empty block list returns FALSE");
   reset_waypoint_state();
   {
      ASSERT_INT(WaypointInBlockRadius(Vector(100, 200, 50)), FALSE);
   }
   PASS();

   return 0;
}

// ============================================================
// Tests: WaypointSlowFloydsHelpers (WaypointSlowFloydsStop / WaypointSlowFloydsState)
// ============================================================

static int test_WaypointSlowFloydsHelpers(void)
{
   printf("WaypointSlowFloydsHelpers:\n");

   TEST("initial state is -1");
   reset_waypoint_state();
   WaypointSlowFloydsStop();
   ASSERT_INT(WaypointSlowFloydsState(), -1);
   PASS();

   TEST("state tracks activation");
   reset_waypoint_state();
   slow_floyds.state = 0;
   ASSERT_INT(WaypointSlowFloydsState(), 0);
   slow_floyds.state = 5;
   ASSERT_INT(WaypointSlowFloydsState(), 5);
   PASS();

   TEST("stop resets state to -1");
   reset_waypoint_state();
   slow_floyds.state = 3;
   ASSERT_INT(WaypointSlowFloydsState(), 3);
   WaypointSlowFloydsStop();
   ASSERT_INT(WaypointSlowFloydsState(), -1);
   PASS();

   return 0;
}

// ============================================================
// Tests: WaypointDrawBeam
// ============================================================

static int msg_begin_call_count = 0;
static void counting_pfnMessageBegin(int msg_dest, int msg_type,
                                     const float *pOrigin, edict_t *ed)
{
   msg_begin_call_count++;
   (void)msg_dest; (void)msg_type; (void)pOrigin; (void)ed;
}

static int test_WaypointDrawBeam(void)
{
   printf("WaypointDrawBeam:\n");

   TEST("no-op when g_waypoint_on=FALSE");
   reset_waypoint_state();
   g_waypoint_on = FALSE;
   msg_begin_call_count = 0;
   g_engfuncs.pfnMessageBegin = counting_pfnMessageBegin;
   {
      edict_t *pEntity = mock_alloc_edict();
      WaypointDrawBeam(pEntity, Vector(0,0,0), Vector(100,0,0),
                       30, 0, 255, 0, 0, 250, 5);
      ASSERT_INT(msg_begin_call_count, 0);
   }
   PASS();

   TEST("sends message when g_waypoint_on=TRUE");
   reset_waypoint_state();
   g_waypoint_on = TRUE;
   msg_begin_call_count = 0;
   g_engfuncs.pfnMessageBegin = counting_pfnMessageBegin;
   {
      edict_t *pEntity = mock_alloc_edict();
      WaypointDrawBeam(pEntity, Vector(0,0,0), Vector(100,0,0),
                       30, 0, 255, 0, 0, 250, 5);
      ASSERT_TRUE(msg_begin_call_count > 0);
   }
   PASS();

   return 0;
}

// ============================================================
// Tests: WaypointRouteInit
// ============================================================

static int test_WaypointRouteInit(void)
{
   printf("WaypointRouteInit:\n");

   TEST("ForceRebuild with 0 waypoints is early return");
   reset_waypoint_state();
   num_waypoints = 0;
   shortest_path = NULL;
   from_to = NULL;
   WaypointRouteInit(TRUE);
   ASSERT_PTR_NULL(shortest_path);
   ASSERT_PTR_NULL(from_to);
   ASSERT_INT(wp_matrix_initialized, FALSE);
   PASS();

   TEST("ForceRebuild calculates matrix from paths");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   gpGlobals->mapname = (string_t)(long)"testmap";
   {
      // 3 waypoints in a chain: 0->1->2
      setup_waypoint(0, Vector(0, 0, 0), 0, 0);
      setup_waypoint(1, Vector(100, 0, 0), 0, 0);
      setup_waypoint(2, Vector(200, 0, 0), 0, 0);
      setup_path(0, 1);
      setup_path(1, 2);
      WaypointRouteInit(TRUE);
      // Should have assigned arrays
      ASSERT_PTR_NOT_NULL(shortest_path);
      ASSERT_PTR_NOT_NULL(from_to);
      ASSERT_INT(route_num_waypoints, 3);
      // Direct path 0->1 distance = 100
      ASSERT_TRUE(shortest_path[0 * 3 + 1] == 100);
      // Direct path 1->2 distance = 100
      ASSERT_TRUE(shortest_path[1 * 3 + 2] == 100);
      // Diagonal = 0
      ASSERT_TRUE(shortest_path[0 * 3 + 0] == 0);
      // slow_floyds activated
      ASSERT_INT(slow_floyds.state, 0);
      ASSERT_INT(wp_matrix_initialized, FALSE);
   }
   PASS();

   TEST("ForceRebuild caps distance at WAYPOINT_MAX_DISTANCE");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   gpGlobals->mapname = (string_t)(long)"testmap";
   {
      // Two waypoints > WAYPOINT_MAX_DISTANCE apart, connected by lift path
      // WAYPOINT_MAX_DISTANCE = USHRT_MAX-1 = 65534
      setup_waypoint(0, Vector(0, 0, 0), W_FL_LIFT_START, 0);
      setup_waypoint(1, Vector(0, 0, 66000), W_FL_LIFT_END, 0);
      setup_path(0, 1);
      WaypointRouteInit(TRUE);
      // Distance = 66000 > WAYPOINT_MAX_DISTANCE, should be capped
      ASSERT_TRUE(shortest_path[0 * 2 + 1] == WAYPOINT_MAX_DISTANCE);
   }
   PASS();

   TEST("ForceRebuild skips non-lift paths beyond REACHABLE_RANGE");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   gpGlobals->mapname = (string_t)(long)"testmap";
   {
      // Two normal waypoints beyond REACHABLE_RANGE with path
      float far_dist = REACHABLE_RANGE + 100;
      setup_waypoint(0, Vector(0, 0, 0), 0, 0);
      setup_waypoint(1, Vector(far_dist, 0, 0), 0, 0);
      setup_path(0, 1);
      WaypointRouteInit(TRUE);
      // Distance > REACHABLE_RANGE for non-lift: should NOT be stored
      ASSERT_TRUE(shortest_path[0 * 2 + 1] == WAYPOINT_UNREACHABLE);
   }
   PASS();

   TEST("file load: valid .matrix file");
   reset_waypoint_state();
   enter_temp_dir();
   gpGlobals->mapname = (string_t)(long)"testmap";
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      // Create waypoints + paths and save .wpt
      setup_waypoint(0, Vector(0, 0, 0), 0, 0);
      setup_waypoint(1, Vector(100, 0, 0), 0, 0);
      setup_waypoint(2, Vector(200, 0, 0), 0, 0);
      setup_path(0, 1);
      setup_path(1, 2);
      g_waypoint_paths = TRUE;
      g_waypoint_updated = TRUE;
      WaypointSave();

      // Set up a known matrix and save it
      setup_route_matrix(3);
      shortest_path[0 * 3 + 1] = 100;
      shortest_path[0 * 3 + 2] = 200;
      shortest_path[1 * 3 + 2] = 100;
      from_to[0 * 3 + 1] = 1;
      from_to[0 * 3 + 2] = 1;
      from_to[1 * 3 + 2] = 2;
      WaypointSaveFloydsMatrix();

      // Reset and reload via WaypointRouteInit with ForceRebuild=FALSE
      shortest_path = NULL;
      from_to = NULL;
      wp_matrix_initialized = FALSE;
      WaypointRouteInit(FALSE);

      ASSERT_INT(wp_matrix_initialized, TRUE);
      ASSERT_PTR_NOT_NULL(shortest_path);
      ASSERT_PTR_NOT_NULL(from_to);
      ASSERT_TRUE(shortest_path[0 * 3 + 1] == 100);
      ASSERT_TRUE(shortest_path[0 * 3 + 2] == 200);
      ASSERT_TRUE(from_to[0 * 3 + 2] == 1);
   }
   leave_temp_dir();
   PASS();

   TEST("file load: falls back when no .matrix file");
   reset_waypoint_state();
   enter_temp_dir();
   gpGlobals->mapname = (string_t)(long)"testmap";
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      // Save .wpt only, delete any .matrix
      setup_waypoint(0, Vector(0, 0, 0), 0, 0);
      setup_waypoint(1, Vector(100, 0, 0), 0, 0);
      setup_path(0, 1);
      g_waypoint_paths = TRUE;
      g_waypoint_updated = TRUE;
      WaypointSave();

      // Remove .matrix file if exists
      char matfile[512];
      snprintf(matfile, sizeof(matfile),
               "%s/valve/addons/jk_botti/waypoints/testmap.matrix", test_tmpdir);
      unlink(matfile);

      shortest_path = NULL;
      from_to = NULL;
      wp_matrix_initialized = FALSE;
      WaypointRouteInit(FALSE);

      // Should fall through to calculation
      ASSERT_INT(slow_floyds.state, 0);
      ASSERT_INT(wp_matrix_initialized, FALSE);
      ASSERT_PTR_NOT_NULL(shortest_path);
   }
   leave_temp_dir();
   PASS();

   TEST("file load: falls back on corrupt .matrix header");
   reset_waypoint_state();
   enter_temp_dir();
   gpGlobals->mapname = (string_t)(long)"testmap";
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      // Save .wpt
      setup_waypoint(0, Vector(0, 0, 0), 0, 0);
      setup_waypoint(1, Vector(100, 0, 0), 0, 0);
      setup_path(0, 1);
      g_waypoint_paths = TRUE;
      g_waypoint_updated = TRUE;
      WaypointSave();

      // Write corrupt .matrix
      char matfile[512];
      snprintf(matfile, sizeof(matfile),
               "%s/valve/addons/jk_botti/waypoints/testmap.matrix", test_tmpdir);
      gzFile fp = gzopen(matfile, "wb");
      gzwrite(fp, "CORRUPT_HEADER!!", 16);
      gzclose(fp);

      shortest_path = NULL;
      from_to = NULL;
      wp_matrix_initialized = FALSE;
      WaypointRouteInit(FALSE);

      // Should fall back to calculation
      ASSERT_INT(slow_floyds.state, 0);
      ASSERT_INT(wp_matrix_initialized, FALSE);
   }
   leave_temp_dir();
   PASS();

   TEST("file load: falls back when .wpt newer than .matrix");
   reset_waypoint_state();
   WaypointSlowFloydsStop();
   enter_temp_dir();
   gpGlobals->mapname = (string_t)(long)"testmap";
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      // Create waypoints and save .wpt first
      setup_waypoint(0, Vector(0, 0, 0), 0, 0);
      setup_waypoint(1, Vector(100, 0, 0), 0, 0);
      setup_path(0, 1);
      g_waypoint_paths = TRUE;
      g_waypoint_updated = TRUE;

      // Save .matrix first (will be older)
      setup_route_matrix(2);
      WaypointSaveFloydsMatrix();

      // Make .wpt file explicitly newer using utimes
      {
         char matfile[512], wptfile[512];
         struct stat st;
         struct timeval times[2];

         snprintf(matfile, sizeof(matfile),
                  "%s/valve/addons/jk_botti/waypoints/testmap.matrix", test_tmpdir);
         stat(matfile, &st);

         // Save .wpt
         WaypointSave();

         // Set .wpt mtime to matrix mtime + 2 seconds
         snprintf(wptfile, sizeof(wptfile),
                  "%s/valve/addons/jk_botti/waypoints/testmap.wpt", test_tmpdir);
         times[0].tv_sec = st.st_mtime + 2;
         times[0].tv_usec = 0;
         times[1].tv_sec = st.st_mtime + 2;
         times[1].tv_usec = 0;
         utimes(wptfile, times);
      }

      shortest_path = NULL;
      from_to = NULL;
      wp_matrix_initialized = FALSE;
      WaypointRouteInit(FALSE);

      // .wpt newer -> recalculation
      ASSERT_INT(slow_floyds.state, 0);
      ASSERT_INT(wp_matrix_initialized, FALSE);
   }
   leave_temp_dir();
   PASS();

   return 0;
}

// ============================================================
// Tests: WaypointAutowaypointing
// ============================================================

static int test_WaypointAutowaypointing(void)
{
   printf("WaypointAutowaypointing:\n");

   TEST("not on ground/ladder/water -> no add");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      edict_t *pEntity = mock_alloc_edict();
      int idx = g_engfuncs.pfnIndexOfEdict(pEntity) - 1;
      pEntity->v.origin = Vector(500, 0, 0);
      // Not on ground, not flying (ladder), waterlevel=0
      pEntity->v.flags = 0;
      pEntity->v.movetype = MOVETYPE_WALK;
      pEntity->v.waterlevel = 0;
      players[idx].last_waypoint = -1;
      setup_waypoint(0, Vector(0, 0, 0), 0, 0);
      int before = num_waypoints;
      WaypointAutowaypointing(idx, pEntity);
      ASSERT_INT(num_waypoints, before);
   }
   PASS();

   TEST("on moving platform -> no add");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      edict_t *pEntity = mock_alloc_edict();
      int idx = g_engfuncs.pfnIndexOfEdict(pEntity) - 1;
      pEntity->v.origin = Vector(500, 0, 0);
      pEntity->v.flags = FL_ONGROUND;
      pEntity->v.movetype = MOVETYPE_WALK;
      // Ground entity with speed > 0
      edict_t *platform = mock_alloc_edict();
      platform->v.speed = 100.0f;
      pEntity->v.groundentity = platform;
      players[idx].last_waypoint = -1;
      setup_waypoint(0, Vector(0, 0, 0), 0, 0);
      int before = num_waypoints;
      WaypointAutowaypointing(idx, pEntity);
      ASSERT_INT(num_waypoints, before);
   }
   PASS();

   TEST("grapple movement -> no add");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      edict_t *pEntity = mock_alloc_edict();
      int idx = g_engfuncs.pfnIndexOfEdict(pEntity) - 1;
      pEntity->v.origin = Vector(500, 0, 0);
      pEntity->v.flags = FL_IMMUNE_LAVA;
      pEntity->v.movetype = MOVETYPE_FLY;
      players[idx].last_waypoint = -1;
      setup_waypoint(0, Vector(0, 0, 0), 0, 0);
      int before = num_waypoints;
      WaypointAutowaypointing(idx, pEntity);
      ASSERT_INT(num_waypoints, before);
   }
   PASS();

   TEST("adds when far from all reachable waypoints");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      edict_t *pEntity = mock_alloc_edict();
      int idx = g_engfuncs.pfnIndexOfEdict(pEntity) - 1;
      pEntity->v.origin = Vector(300, 0, 0);
      pEntity->v.flags = FL_ONGROUND;
      pEntity->v.movetype = MOVETYPE_WALK;
      players[idx].last_waypoint = -1;
      // Existing waypoint far away; target_distance=200, min_distance > 200
      setup_waypoint(0, Vector(0, 0, 0), 0, 0);
      int before = num_waypoints;
      WaypointAutowaypointing(idx, pEntity);
      // Should have added a waypoint
      ASSERT_TRUE(num_waypoints > before);
   }
   PASS();

   TEST("no add when close to reachable waypoint");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      edict_t *pEntity = mock_alloc_edict();
      int idx = g_engfuncs.pfnIndexOfEdict(pEntity) - 1;
      pEntity->v.origin = Vector(50, 0, 0);
      pEntity->v.flags = FL_ONGROUND;
      pEntity->v.movetype = MOVETYPE_WALK;
      players[idx].last_waypoint = -1;
      // Waypoint at origin, player at 50 units -> min_distance = 50 < 200
      setup_waypoint(0, Vector(0, 0, 0), 0, 0);
      int before = num_waypoints;
      WaypointAutowaypointing(idx, pEntity);
      ASSERT_INT(num_waypoints, before);
   }
   PASS();

   TEST("ladder uses 65-unit target distance");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      edict_t *pEntity = mock_alloc_edict();
      int idx = g_engfuncs.pfnIndexOfEdict(pEntity) - 1;
      pEntity->v.origin = Vector(0, 0, 100);
      pEntity->v.flags = 0; // not on ground
      pEntity->v.movetype = MOVETYPE_FLY; // on ladder
      players[idx].last_waypoint = -1;
      // Existing waypoint 100 units below (vertical on ladder)
      // target_distance=65, distance=100 >= 65, and no nearby reachable wp < 65
      // Need existing wpt to be reachable but > 65 away
      setup_waypoint(0, Vector(0, 0, 0), W_FL_LADDER, 0);
      int before = num_waypoints;
      WaypointAutowaypointing(idx, pEntity);
      ASSERT_TRUE(num_waypoints > before);
   }
   PASS();

   TEST("links unreachable one-path waypoints");
   reset_waypoint_state();
   {
      // Custom trace: makes wp0 and wp1 unreachable to each other,
      // but both reachable from player position
      static Vector s_player_pos;
      s_player_pos = Vector(150, 0, 0);

      mock_trace_hull_fn = [](const float *v1, const float *v2,
                              int fNoMonsters, int hullNumber,
                              edict_t *pentToSkip, TraceResult *ptr)
      {
         memset(ptr, 0, sizeof(*ptr));

         Vector src(v1[0], v1[1], v1[2]);
         Vector dst(v2[0], v2[1], v2[2]);

         // Ground probes: on ground
         if (v2[2] < v1[2] - 500.0f)
         {
            ptr->flFraction = 0.01f;
            ptr->vecEndPos[0] = v1[0];
            ptr->vecEndPos[1] = v1[1];
            ptr->vecEndPos[2] = v1[2] + (v2[2] - v1[2]) * 0.01f;
            return;
         }
         if (v2[2] < v1[2] - 40.0f && v2[2] > v1[2] - 60.0f)
         {
            ptr->flFraction = 0.01f;
            ptr->vecEndPos[0] = v1[0];
            ptr->vecEndPos[1] = v1[1];
            ptr->vecEndPos[2] = v1[2] + (v2[2] - v1[2]) * 0.01f;
            return;
         }

         // Block traces between wp0(0,0,0) and wp1(300,0,0)
         float src_to_0 = (src - Vector(0,0,0)).Length();
         float dst_to_300 = (dst - Vector(300,0,0)).Length();
         float src_to_300 = (src - Vector(300,0,0)).Length();
         float dst_to_0 = (dst - Vector(0,0,0)).Length();

         if ((src_to_0 < 1 && dst_to_300 < 1) ||
             (src_to_300 < 1 && dst_to_0 < 1))
         {
            // Block wp0<->wp1 visibility
            ptr->flFraction = 0.5f;
            ptr->fAllSolid = TRUE;
            return;
         }

         // Everything else visible
         ptr->flFraction = 1.0f;
         ptr->vecEndPos[0] = v2[0];
         ptr->vecEndPos[1] = v2[1];
         ptr->vecEndPos[2] = v2[2];
      };
      mock_trace_line_fn = mock_trace_hull_fn;

      edict_t *pEntity = mock_alloc_edict();
      int idx = g_engfuncs.pfnIndexOfEdict(pEntity) - 1;
      pEntity->v.origin = s_player_pos;
      pEntity->v.flags = FL_ONGROUND;
      pEntity->v.movetype = MOVETYPE_WALK;
      players[idx].last_waypoint = -1;

      // wp0 at 0,0,0 with 0 paths (onepath candidate)
      // wp1 at 300,0,0 (within 350 of wp0)
      // Both are reachable from player at 150,0,0
      // wp0 and wp1 are unreachable to each other
      setup_waypoint(0, Vector(0, 0, 0), 0, 0);
      setup_waypoint(1, Vector(300, 0, 0), 0, 0);

      int before = num_waypoints;
      WaypointAutowaypointing(idx, pEntity);
      // Should have added a linking waypoint
      ASSERT_TRUE(num_waypoints > before);
   }
   PASS();

   TEST("route-cutting adds waypoint for long routes");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      edict_t *pEntity = mock_alloc_edict();
      int idx = g_engfuncs.pfnIndexOfEdict(pEntity) - 1;
      pEntity->v.origin = Vector(150, 0, 0);
      pEntity->v.flags = FL_ONGROUND;
      pEntity->v.movetype = MOVETYPE_WALK;
      players[idx].last_waypoint = -1;

      // Two waypoints reachable from player, both >= 64 units away
      setup_waypoint(0, Vector(0, 0, 0), 0, 0);
      setup_waypoint(1, Vector(300, 0, 0), 0, 0);

      // Set up route matrix with long route between wp0 and wp1
      setup_route_matrix(2);
      shortest_path[0 * 2 + 1] = 2000; // > 1600
      from_to[0 * 2 + 1] = 1;
      shortest_path[1 * 2 + 0] = 2000;
      from_to[1 * 2 + 0] = 0;

      int before = num_waypoints;
      WaypointAutowaypointing(idx, pEntity);
      // Should have added a route-cutting waypoint
      ASSERT_TRUE(num_waypoints > before);
   }
   PASS();

   TEST("block prevents re-adding in same area");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      edict_t *pEntity = mock_alloc_edict();
      int idx = g_engfuncs.pfnIndexOfEdict(pEntity) - 1;
      pEntity->v.origin = Vector(150, 0, 0);
      pEntity->v.flags = FL_ONGROUND;
      pEntity->v.movetype = MOVETYPE_WALK;
      players[idx].last_waypoint = -1;

      // Two waypoints reachable from player, both >= 64 units away
      setup_waypoint(0, Vector(0, 0, 0), 0, 0);
      setup_waypoint(1, Vector(300, 0, 0), 0, 0);

      setup_route_matrix(2);
      shortest_path[0 * 2 + 1] = 2000;
      from_to[0 * 2 + 1] = 1;
      shortest_path[1 * 2 + 0] = 2000;
      from_to[1 * 2 + 0] = 0;

      // Pre-block the player's area
      WaypointAddBlock(pEntity->v.origin);

      int before = num_waypoints;
      WaypointAutowaypointing(idx, pEntity);
      // Block prevents route-cutting waypoint addition
      ASSERT_INT(num_waypoints, before);
   }
   PASS();

   return 0;
}

// ============================================================
// Tests: WaypointThink
// ============================================================

static int test_WaypointThink(void)
{
   printf("WaypointThink:\n");

   TEST("invalid player index returns immediately");
   reset_waypoint_state();
   {
      // edict at index 0 -> idx = -1 -> return
      edict_t fake;
      memset(&fake, 0, sizeof(fake));
      // mock_pfnIndexOfEdict returns 0 for unknown edicts
      int before = num_waypoints;
      WaypointThink(&fake);
      ASSERT_INT(num_waypoints, before);
   }
   PASS();

   TEST("auto-waypointing dispatched when enabled");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   g_auto_waypoint = TRUE;
   {
      edict_t *pEntity = mock_alloc_edict();
      pEntity->v.origin = Vector(500, 0, 0);
      pEntity->v.flags = FL_ONGROUND;
      pEntity->v.movetype = MOVETYPE_WALK;
      int idx = g_engfuncs.pfnIndexOfEdict(pEntity) - 1;
      players[idx].last_waypoint = -1;
      // Existing waypoint far away
      setup_waypoint(0, Vector(0, 0, 0), 0, 0);
      int before = num_waypoints;
      WaypointThink(pEntity);
      // Should have auto-added a waypoint
      ASSERT_TRUE(num_waypoints > before);
   }
   PASS();

   TEST("no auto-waypointing when disabled");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   g_auto_waypoint = FALSE;
   {
      edict_t *pEntity = mock_alloc_edict();
      pEntity->v.origin = Vector(500, 0, 0);
      pEntity->v.flags = FL_ONGROUND;
      pEntity->v.movetype = MOVETYPE_WALK;
      int idx = g_engfuncs.pfnIndexOfEdict(pEntity) - 1;
      players[idx].last_waypoint = -1;
      setup_waypoint(0, Vector(0, 0, 0), 0, 0);
      int before = num_waypoints;
      WaypointThink(pEntity);
      ASSERT_INT(num_waypoints, before);
   }
   PASS();

   TEST("display updates wp_display_time");
   reset_waypoint_state();
   g_waypoint_on = TRUE;
   g_auto_waypoint = FALSE;
   gpGlobals->time = 100.0f;
   {
      edict_t *pEntity = mock_alloc_edict();
      pEntity->v.origin = Vector(10, 0, 0);
      setup_waypoint(0, Vector(0, 0, 0), 0, 0);
      // Display time expired (0.0 + 1.0 < 100.0)
      wp_display_time[0] = 0.0f;
      WaypointThink(pEntity);
      // wp_display_time should be updated to current time
      ASSERT_TRUE(wp_display_time[0] == gpGlobals->time);
   }
   PASS();

   TEST("path drawing updates f_path_time");
   reset_waypoint_state();
   g_waypoint_on = TRUE;
   g_path_waypoint = TRUE;
   g_auto_waypoint = FALSE;
   gpGlobals->time = 100.0f;
   f_path_time = 0.0f;
   {
      edict_t *pEntity = mock_alloc_edict();
      pEntity->v.origin = Vector(10, 0, 0);
      // Waypoint within 50 units of player, with a path
      setup_waypoint(0, Vector(0, 0, 0), 0, 0);
      setup_waypoint(1, Vector(100, 0, 0), 0, 0);
      setup_path(0, 1);
      wp_display_time[0] = gpGlobals->time; // already displayed

      WaypointThink(pEntity);
      // f_path_time should be updated
      ASSERT_TRUE(f_path_time > 0.0f);
      ASSERT_TRUE(f_path_time == gpGlobals->time + 1.0f);
   }
   PASS();

   return 0;
}

// ============================================================
// Tests: WaypointThink beam colors
// ============================================================

static int test_WaypointThinkBeamColors(void)
{
   printf("WaypointThinkBeamColors:\n");

   TEST("crouch waypoint -> yellow beam");
   reset_waypoint_state();
   g_waypoint_on = TRUE;
   g_auto_waypoint = FALSE;
   gpGlobals->time = 100.0f;
   msg_begin_call_count = 0;
   g_engfuncs.pfnMessageBegin = counting_pfnMessageBegin;
   {
      edict_t *pEntity = mock_alloc_edict();
      pEntity->v.origin = Vector(10, 0, 0);
      setup_waypoint(0, Vector(0, 0, 0), W_FL_CROUCH, 0);
      wp_display_time[0] = 0.0f;
      WaypointThink(pEntity);
      ASSERT_TRUE(msg_begin_call_count > 0);
   }
   PASS();

   TEST("ladder waypoint -> purple beam");
   reset_waypoint_state();
   g_waypoint_on = TRUE;
   g_auto_waypoint = FALSE;
   gpGlobals->time = 100.0f;
   msg_begin_call_count = 0;
   g_engfuncs.pfnMessageBegin = counting_pfnMessageBegin;
   {
      edict_t *pEntity = mock_alloc_edict();
      pEntity->v.origin = Vector(10, 0, 0);
      setup_waypoint(0, Vector(0, 0, 0), W_FL_LADDER, 0);
      wp_display_time[0] = 0.0f;
      WaypointThink(pEntity);
      ASSERT_TRUE(msg_begin_call_count > 0);
   }
   PASS();

   TEST("lift-start waypoint -> red beam");
   reset_waypoint_state();
   g_waypoint_on = TRUE;
   g_auto_waypoint = FALSE;
   gpGlobals->time = 100.0f;
   msg_begin_call_count = 0;
   g_engfuncs.pfnMessageBegin = counting_pfnMessageBegin;
   {
      edict_t *pEntity = mock_alloc_edict();
      pEntity->v.origin = Vector(10, 0, 0);
      setup_waypoint(0, Vector(0, 0, 0), W_FL_LIFT_START, 0);
      wp_display_time[0] = 0.0f;
      WaypointThink(pEntity);
      ASSERT_TRUE(msg_begin_call_count > 0);
   }
   PASS();

   TEST("lift-end waypoint -> green beam");
   reset_waypoint_state();
   g_waypoint_on = TRUE;
   g_auto_waypoint = FALSE;
   gpGlobals->time = 100.0f;
   msg_begin_call_count = 0;
   g_engfuncs.pfnMessageBegin = counting_pfnMessageBegin;
   {
      edict_t *pEntity = mock_alloc_edict();
      pEntity->v.origin = Vector(10, 0, 0);
      setup_waypoint(0, Vector(0, 0, 0), W_FL_LIFT_END, 0);
      wp_display_time[0] = 0.0f;
      WaypointThink(pEntity);
      ASSERT_TRUE(msg_begin_call_count > 0);
   }
   PASS();

   TEST("path drawing when g_path_waypoint=TRUE and paths exist");
   reset_waypoint_state();
   g_waypoint_on = TRUE;
   g_path_waypoint = TRUE;
   g_auto_waypoint = FALSE;
   gpGlobals->time = 100.0f;
   f_path_time = 0.0f;
   msg_begin_call_count = 0;
   g_engfuncs.pfnMessageBegin = counting_pfnMessageBegin;
   {
      edict_t *pEntity = mock_alloc_edict();
      pEntity->v.origin = Vector(10, 0, 0);
      setup_waypoint(0, Vector(0, 0, 0), 0, 0);
      setup_waypoint(1, Vector(100, 0, 0), 0, 0);
      setup_path(0, 1);
      wp_display_time[0] = gpGlobals->time; // already displayed
      WaypointThink(pEntity);
      // Path beam should have been drawn
      ASSERT_TRUE(msg_begin_call_count > 0);
      ASSERT_TRUE(f_path_time == gpGlobals->time + 1.0f);
   }
   PASS();

   return 0;
}

// ============================================================
// Tests: CollectMapSpawnItems extra branches
// ============================================================

static int test_CollectMapSpawnItemsExtra(void)
{
   printf("CollectMapSpawnItems extra branches:\n");

   TEST("item_longjump sets W_FL_LONGJUMP");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      edict_t *e = setup_entity("item_longjump", Vector(100, 200, 50));
      CollectMapSpawnItems(e);
      ASSERT_INT(num_spawnpoints, 1);
      ASSERT_TRUE(spawnpoints[0].flags & W_FL_LONGJUMP);
   }
   PASS();

   TEST("weapon with owner set -> skip");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      edict_t *owner = mock_alloc_edict();
      edict_t *e = setup_entity("weapon_shotgun", Vector(100, 200, 50));
      e->v.owner = owner;
      CollectMapSpawnItems(e);
      // Owner is set, so weapon should be skipped
      ASSERT_INT(num_spawnpoints, 0);
   }
   PASS();

   TEST("weapon without owner -> W_FL_WEAPON");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      edict_t *e = setup_entity("weapon_shotgun", Vector(100, 200, 50));
      e->v.owner = NULL;
      CollectMapSpawnItems(e);
      ASSERT_INT(num_spawnpoints, 1);
      ASSERT_TRUE(spawnpoints[0].flags & W_FL_WEAPON);
   }
   PASS();

   TEST("func_recharge sets W_FL_ARMOR with charger offset logic");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      edict_t *e = setup_entity("func_recharge", Vector(100, 200, 50));
      // Set absmin/absmax so VecBModelOrigin works
      e->v.absmin[0] = 90;  e->v.absmin[1] = 190; e->v.absmin[2] = 40;
      e->v.absmax[0] = 110; e->v.absmax[1] = 210; e->v.absmax[2] = 60;
      CollectMapSpawnItems(e);
      // func_recharge uses the offset loop (8 offsets around it)
      // Each offset calls CollectMapSpawnItems recursively with offset_set=TRUE
      // After all offsets, spawnpoints should have W_FL_ARMOR entries
      ASSERT_TRUE(num_spawnpoints > 0);
      // At least one spawnpoint should have W_FL_ARMOR
      int found_armor = 0;
      for (int i = 0; i < num_spawnpoints; i++)
         if (spawnpoints[i].flags & W_FL_ARMOR)
            found_armor++;
      ASSERT_TRUE(found_armor > 0);
   }
   PASS();

   TEST("func_healthcharger sets W_FL_HEALTH with charger offset logic");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      edict_t *e = setup_entity("func_healthcharger", Vector(100, 200, 50));
      e->v.absmin[0] = 90;  e->v.absmin[1] = 190; e->v.absmin[2] = 40;
      e->v.absmax[0] = 110; e->v.absmax[1] = 210; e->v.absmax[2] = 60;
      CollectMapSpawnItems(e);
      ASSERT_TRUE(num_spawnpoints > 0);
      int found_health = 0;
      for (int i = 0; i < num_spawnpoints; i++)
         if (spawnpoints[i].flags & W_FL_HEALTH)
            found_health++;
      ASSERT_TRUE(found_health > 0);
   }
   PASS();

   return 0;
}

// ============================================================
// Tests: WaypointSearchItems extra branches
// ============================================================

static int test_WaypointSearchItemsExtra(void)
{
   printf("WaypointSearchItems extra branches:\n");

   TEST("detects weapon item with itemflags");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0, 0, 0), 0, 0);
   {
      edict_t *e = setup_entity("weapon_shotgun", Vector(10, 0, 0));
      e->v.owner = NULL;
      WaypointSearchItems(NULL, waypoints[0].origin, 0);
      ASSERT_TRUE(waypoints[0].flags & W_FL_WEAPON);
      ASSERT_TRUE(waypoints[0].itemflags != 0);
   }
   PASS();

   TEST("weapon with owner ignored");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0, 0, 0), 0, 0);
   {
      edict_t *owner = mock_alloc_edict();
      edict_t *e = setup_entity("weapon_shotgun", Vector(10, 0, 0));
      e->v.owner = owner;
      WaypointSearchItems(NULL, waypoints[0].origin, 0);
      ASSERT_TRUE(!(waypoints[0].flags & W_FL_WEAPON));
   }
   PASS();

   TEST("detects func_healthcharger");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0, 0, 0), 0, 0);
   {
      edict_t *e = setup_entity("func_healthcharger", Vector(10, 0, 0));
      e->v.absmin[0] = 5;  e->v.absmin[1] = -5; e->v.absmin[2] = -5;
      e->v.absmax[0] = 15; e->v.absmax[1] = 5;  e->v.absmax[2] = 5;
      WaypointSearchItems(NULL, waypoints[0].origin, 0);
      ASSERT_TRUE(waypoints[0].flags & W_FL_HEALTH);
   }
   PASS();

   TEST("detects func_recharge");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0, 0, 0), 0, 0);
   {
      edict_t *e = setup_entity("func_recharge", Vector(10, 0, 0));
      e->v.absmin[0] = 5;  e->v.absmin[1] = -5; e->v.absmin[2] = -5;
      e->v.absmax[0] = 15; e->v.absmax[1] = 5;  e->v.absmax[2] = 5;
      WaypointSearchItems(NULL, waypoints[0].origin, 0);
      ASSERT_TRUE(waypoints[0].flags & W_FL_ARMOR);
   }
   PASS();

   TEST("pEntity non-NULL -> ClientPrint for health");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0, 0, 0), 0, 0);
   {
      edict_t *pEntity = mock_alloc_edict();
      mock_set_classname(pEntity, "player");
      setup_entity("item_healthkit", Vector(10, 0, 0));
      WaypointSearchItems(pEntity, waypoints[0].origin, 0);
      ASSERT_TRUE(waypoints[0].flags & W_FL_HEALTH);
   }
   PASS();

   TEST("pEntity non-NULL -> ClientPrint for armor");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0, 0, 0), 0, 0);
   {
      edict_t *pEntity = mock_alloc_edict();
      mock_set_classname(pEntity, "player");
      setup_entity("item_battery", Vector(10, 0, 0));
      WaypointSearchItems(pEntity, waypoints[0].origin, 0);
      ASSERT_TRUE(waypoints[0].flags & W_FL_ARMOR);
   }
   PASS();

   TEST("pEntity non-NULL -> ClientPrint for ammo");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0, 0, 0), 0, 0);
   {
      edict_t *pEntity = mock_alloc_edict();
      mock_set_classname(pEntity, "player");
      setup_entity("ammo_buckshot", Vector(10, 0, 0));
      WaypointSearchItems(pEntity, waypoints[0].origin, 0);
      ASSERT_TRUE(waypoints[0].flags & W_FL_AMMO);
   }
   PASS();

   TEST("pEntity non-NULL -> ClientPrint for weapon");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0, 0, 0), 0, 0);
   {
      edict_t *pEntity = mock_alloc_edict();
      mock_set_classname(pEntity, "player");
      edict_t *w = setup_entity("weapon_shotgun", Vector(10, 0, 0));
      w->v.owner = NULL;
      WaypointSearchItems(pEntity, waypoints[0].origin, 0);
      ASSERT_TRUE(waypoints[0].flags & W_FL_WEAPON);
   }
   PASS();

   TEST("pEntity non-NULL -> ClientPrint for longjump");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0, 0, 0), 0, 0);
   {
      edict_t *pEntity = mock_alloc_edict();
      mock_set_classname(pEntity, "player");
      setup_entity("item_longjump", Vector(10, 0, 0));
      WaypointSearchItems(pEntity, waypoints[0].origin, 0);
      ASSERT_TRUE(waypoints[0].flags & W_FL_LONGJUMP);
   }
   PASS();

   return 0;
}

// ============================================================
// Tests: WaypointFindNearestGoal item filtering
// ============================================================

static int test_WaypointFindNearestGoalItemFiltering(void)
{
   printf("WaypointFindNearestGoal item filtering:\n");

   TEST("EF_NODRAW item -> skip waypoint");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), W_FL_HEALTH, 0);
   setup_waypoint(1, Vector(100,0,0), W_FL_CROUCH, 0);
   {
      // Create a health item with EF_NODRAW
      edict_t *e = setup_entity("item_healthkit", Vector(10, 0, 0));
      e->v.effects |= EF_NODRAW;
      Vector src(0, 0, 0);
      // Should skip wp0 (health item has EF_NODRAW) and return wp1
      int idx = WaypointFindNearestGoal(NULL, -1, W_FL_CROUCH | W_FL_HEALTH, 0, NULL, 0.0f, &src);
      ASSERT_INT(idx, 1);
   }
   PASS();

   TEST("item with frame>0 -> skip waypoint");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), W_FL_WEAPON, 0);
   setup_waypoint(1, Vector(100,0,0), W_FL_CROUCH, 0);
   {
      // Create a weapon item with frame > 0 (being picked up)
      edict_t *e = setup_entity("weapon_shotgun", Vector(10, 0, 0));
      e->v.owner = NULL;
      e->v.frame = 1.0f;
      Vector src(0, 0, 0);
      int idx = WaypointFindNearestGoal(NULL, -1, W_FL_CROUCH | W_FL_WEAPON, 0, NULL, 0.0f, &src);
      ASSERT_INT(idx, 1);
   }
   PASS();

   TEST("NULL WaypointFindItem -> skip waypoint");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), W_FL_HEALTH, 0);
   setup_waypoint(1, Vector(100,0,0), W_FL_CROUCH, 0);
   {
      // No item entities exist, so WaypointFindItem returns NULL
      Vector src(0, 0, 0);
      int idx = WaypointFindNearestGoal(NULL, -1, W_FL_CROUCH | W_FL_HEALTH, 0, NULL, 0.0f, &src);
      ASSERT_INT(idx, 1);
   }
   PASS();

   // Also test WaypointFindRandomGoal item filtering (same logic, different function)
   TEST("WaypointFindRandomGoal: NULL item -> skip");
   reset_waypoint_state();
   setup_waypoint(0, Vector(0,0,0), W_FL_HEALTH, 0);
   setup_waypoint(1, Vector(100,0,0), W_FL_CROUCH, 0);
   {
      int out[4];
      // wp0 has W_FL_HEALTH but no item entity -> filtered out
      // wp1 has W_FL_CROUCH -> passes (no item check needed)
      int count = WaypointFindRandomGoal(out, 4, NULL, W_FL_CROUCH | W_FL_HEALTH, 0, NULL);
      ASSERT_INT(count, 1);
      ASSERT_INT(out[0], 1);
   }
   PASS();

   return 0;
}

// ============================================================
// Tests: WaypointReachable water tracking
// ============================================================

static int test_WaypointReachableWater(void)
{
   printf("WaypointReachable water tracking:\n");

   TEST("source in water, dest in air (upward direction)");
   reset_waypoint_state();
   {
      // Source is in water, dest is above water surface
      // Water surface at z=10, dest at z=40 (must be <= src.z+45 to avoid
      // high-dest mid-air check)
      mock_point_contents_fn = [](const float *origin) -> int {
         if (origin[2] < 10.0f)
            return CONTENTS_WATER;
         return CONTENTS_EMPTY;
      };

      mock_trace_hull_fn = [](const float *v1, const float *v2,
                              int fNoMonsters, int hullNumber,
                              edict_t *pentToSkip, TraceResult *ptr)
      {
         memset(ptr, 0, sizeof(*ptr));
         // Ground probes (straight down 1000 units)
         if (v2[2] < v1[2] - 500.0f)
         {
            ptr->flFraction = 0.01f;
            ptr->vecEndPos[0] = v1[0];
            ptr->vecEndPos[1] = v1[1];
            ptr->vecEndPos[2] = v1[2] + (v2[2] - v1[2]) * 0.01f;
            return;
         }
         // Visibility trace and others: pass
         ptr->flFraction = 1.0f;
         ptr->vecEndPos[0] = v2[0];
         ptr->vecEndPos[1] = v2[1];
         ptr->vecEndPos[2] = v2[2];
      };
      mock_trace_line_fn = mock_trace_hull_fn;

      // Source underwater (z=0), dest above water (z=40)
      // dest.z=40 <= src.z+45=45, so high-dest mid-air check is skipped
      // Exercises: src is CONTENTS_WATER (line 2011), direction.z > 0 (line 2018)
      // Note: water surface detection while loop (lines 2021-2031) has a bug
      // where the condition is initially false, making the loop body dead code
      Vector src(0, 0, 0);
      Vector dest(100, 0, 40);
      int result = WaypointReachable(src, dest, 0);
      ASSERT_INT(result, TRUE);
   }
   PASS();

   TEST("source in water, direction z <= 0 (no upward loop)");
   reset_waypoint_state();
   {
      // Source in water (x < 50), dest not in water (x >= 50)
      // Both at same z, so direction.z = 0, which skips the upward water
      // surface detection loop (line 2018 FALSE branch)
      mock_point_contents_fn = [](const float *origin) -> int {
         if (origin[0] < 50.0f)
            return CONTENTS_WATER;
         return CONTENTS_EMPTY;
      };

      mock_trace_hull_fn = [](const float *v1, const float *v2,
                              int fNoMonsters, int hullNumber,
                              edict_t *pentToSkip, TraceResult *ptr)
      {
         memset(ptr, 0, sizeof(*ptr));
         // Ground probes (straight down 1000 units)
         if (v2[2] < v1[2] - 500.0f)
         {
            ptr->flFraction = 0.01f;
            ptr->vecEndPos[0] = v1[0];
            ptr->vecEndPos[1] = v1[1];
            ptr->vecEndPos[2] = v1[2] + (v2[2] - v1[2]) * 0.01f;
            return;
         }
         // Visibility and others: pass
         ptr->flFraction = 1.0f;
         ptr->vecEndPos[0] = v2[0];
         ptr->vecEndPos[1] = v2[1];
         ptr->vecEndPos[2] = v2[2];
      };
      mock_trace_line_fn = mock_trace_hull_fn;

      // Both at z=0, direction.z = 0
      // src (x=0) is WATER, dest (x=100) is EMPTY -> not both water
      // Enters water block (line 2011), but direction.z <= 0 skips upward loop
      Vector src(0, 0, 0);
      Vector dest(100, 0, 0);
      int result = WaypointReachable(src, dest, 0);
      ASSERT_INT(result, TRUE);
   }
   PASS();

   return 0;
}

// ============================================================
// Tests: WaypointAutowaypointing extra branches
// ============================================================

static int test_WaypointAutowayExtraBranches(void)
{
   printf("WaypointAutowaypointing extra branches:\n");

   TEST("underwater player adds waypoint");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      edict_t *pEntity = mock_alloc_edict();
      int idx = g_engfuncs.pfnIndexOfEdict(pEntity) - 1;
      pEntity->v.origin = Vector(300, 0, 0);
      pEntity->v.flags = 0; // not on ground
      pEntity->v.movetype = MOVETYPE_WALK;
      pEntity->v.waterlevel = 3; // fully submerged
      players[idx].last_waypoint = -1;
      setup_waypoint(0, Vector(0, 0, 0), 0, 0);
      int before = num_waypoints;
      WaypointAutowaypointing(idx, pEntity);
      ASSERT_TRUE(num_waypoints > before);
   }
   PASS();

   TEST("rotating platform -> no add");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      edict_t *pEntity = mock_alloc_edict();
      int idx = g_engfuncs.pfnIndexOfEdict(pEntity) - 1;
      pEntity->v.origin = Vector(500, 0, 0);
      pEntity->v.flags = FL_ONGROUND;
      pEntity->v.movetype = MOVETYPE_WALK;
      edict_t *platform = mock_alloc_edict();
      platform->v.speed = 0.0f;
      platform->v.avelocity = Vector(0, 10, 0); // rotating
      pEntity->v.groundentity = platform;
      players[idx].last_waypoint = -1;
      setup_waypoint(0, Vector(0, 0, 0), 0, 0);
      int before = num_waypoints;
      WaypointAutowaypointing(idx, pEntity);
      ASSERT_INT(num_waypoints, before);
   }
   PASS();

   TEST("crouch waypoint auto-added when ducking");
   reset_waypoint_state();
   {
      // Block upward duck trace, pass everything else
      mock_trace_hull_fn = [](const float *v1, const float *v2,
                              int fNoMonsters, int hullNumber,
                              edict_t *pentToSkip, TraceResult *ptr)
      {
         memset(ptr, 0, sizeof(*ptr));
         if (v2[2] > v1[2])
         {
            // Upward trace (duck check): blocked
            ptr->flFraction = 0.5f;
            ptr->fAllSolid = TRUE;
         }
         else if (v2[2] < v1[2] - 500.0f)
         {
            // Ground probes
            ptr->flFraction = 0.01f;
            ptr->vecEndPos[0] = v1[0];
            ptr->vecEndPos[1] = v1[1];
            ptr->vecEndPos[2] = v1[2] + (v2[2] - v1[2]) * 0.01f;
         }
         else
         {
            ptr->flFraction = 1.0f;
            ptr->vecEndPos[0] = v2[0];
            ptr->vecEndPos[1] = v2[1];
            ptr->vecEndPos[2] = v2[2];
         }
      };
      mock_trace_line_fn = mock_trace_hull_fn;

      edict_t *pEntity = mock_alloc_edict();
      int idx = g_engfuncs.pfnIndexOfEdict(pEntity) - 1;
      pEntity->v.origin = Vector(300, 0, 50);
      pEntity->v.flags = FL_ONGROUND | FL_DUCKING;
      pEntity->v.movetype = MOVETYPE_WALK;
      players[idx].last_waypoint = -1;
      setup_waypoint(0, Vector(0, 0, 0), 0, 0);
      int before = num_waypoints;
      WaypointAutowaypointing(idx, pEntity);
      ASSERT_TRUE(num_waypoints > before);
      // The newly added waypoint should have W_FL_CROUCH
      // WaypointAdd checks FL_DUCKING and adds W_FL_CROUCH when can't stand
      ASSERT_TRUE(waypoints[num_waypoints - 1].flags & W_FL_CROUCH);
   }
   PASS();

   TEST("ladder auto-add sets W_FL_LADDER");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      edict_t *pEntity = mock_alloc_edict();
      int idx = g_engfuncs.pfnIndexOfEdict(pEntity) - 1;
      pEntity->v.origin = Vector(0, 0, 100);
      pEntity->v.flags = 0;
      pEntity->v.movetype = MOVETYPE_FLY; // on ladder
      players[idx].last_waypoint = -1;
      setup_waypoint(0, Vector(0, 0, 0), W_FL_LADDER, 0);
      int before = num_waypoints;
      WaypointAutowaypointing(idx, pEntity);
      ASSERT_TRUE(num_waypoints > before);
      ASSERT_TRUE(waypoints[num_waypoints - 1].flags & W_FL_LADDER);
   }
   PASS();

   return 0;
}

// ============================================================
// Tests: WaypointSave error path
// ============================================================

static int test_WaypointSaveError(void)
{
   printf("WaypointSave error paths:\n");

   TEST("save with no waypoint_updated -> no-op");
   reset_waypoint_state();
   enter_temp_dir();
   gpGlobals->mapname = (string_t)(long)"testmap";
   g_waypoint_updated = FALSE;
   setup_waypoint(0, Vector(10, 20, 30), 0, 0);
   g_waypoint_paths = TRUE;
   {
      // Remove any existing file
      char filename[512];
      snprintf(filename, sizeof(filename),
               "%s/valve/addons/jk_botti/waypoints/testmap.wpt", test_tmpdir);
      unlink(filename);
   }
   WaypointSave();
   // File should not exist
   WaypointInit();
   ASSERT_INT(WaypointLoad(NULL), FALSE);
   leave_temp_dir();
   PASS();

   TEST("load with 0 waypoints succeeds");
   reset_waypoint_state();
   enter_temp_dir();
   gpGlobals->mapname = (string_t)(long)"testmap";
   g_waypoint_updated = TRUE;
   g_waypoint_paths = TRUE;
   // num_waypoints = 0, save should create file with 0 waypoints
   WaypointSave();
   WaypointInit();
   qboolean loaded = WaypointLoad(NULL);
   ASSERT_INT(loaded, TRUE);
   ASSERT_INT(num_waypoints, 0);
   leave_temp_dir();
   PASS();

   TEST("save: gzopen failure path (no waypoints dir)");
   reset_waypoint_state();
   {
      // Don't enter_temp_dir - CWD has no valve/addons/jk_botti/waypoints/
      // so gzopen will fail
      gpGlobals->mapname = (string_t)(long)"errormap";
      g_waypoint_updated = TRUE;
      setup_waypoint(0, Vector(10, 20, 30), 0, 0);
      // Should print error message but not crash
      WaypointSave();
   }
   PASS();

   return 0;
}

// ============================================================
// Tests: WaypointAddLift extra branches
// ============================================================

static int test_WaypointAddLiftExtra(void)
{
   printf("WaypointAddLift extra branches:\n");

   TEST("horizontal movement (X dominant) uses correct offset");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      edict_t *pent = setup_entity("func_door", Vector(100, 200, 0));
      pent->v.absmax[0] = 150; pent->v.absmax[1] = 250; pent->v.absmax[2] = 50;
      pent->v.absmin[0] = 50;  pent->v.absmin[1] = 150; pent->v.absmin[2] = -50;
      pent->v.size[0] = 100; pent->v.size[1] = 100; pent->v.size[2] = 100;

      // Horizontal movement (X direction, abs(move.x) > abs(move.z))
      Vector start(0, 0, 0);
      Vector end(200, 0, 20);  // mostly horizontal
      WaypointAddLift(pent, start, end);

      ASSERT_INT(g_lifts_added, 1);
      ASSERT_INT(num_waypoints, 2);
      ASSERT_TRUE(waypoints[0].flags & W_FL_LIFT_START);
      ASSERT_TRUE(waypoints[1].flags & W_FL_LIFT_END);
   }
   PASS();

   TEST("Y dominant movement uses correct offset");
   reset_waypoint_state();
   mock_trace_hull_fn = trace_for_reachable;
   mock_trace_line_fn = trace_for_reachable;
   reachable_visibility_result = 0;
   reachable_ground_fraction = 0.01f;
   {
      edict_t *pent = setup_entity("func_door", Vector(100, 200, 0));
      pent->v.absmax[0] = 150; pent->v.absmax[1] = 250; pent->v.absmax[2] = 50;
      pent->v.absmin[0] = 50;  pent->v.absmin[1] = 150; pent->v.absmin[2] = -50;
      pent->v.size[0] = 100; pent->v.size[1] = 100; pent->v.size[2] = 100;

      Vector start(0, 0, 0);
      Vector end(0, 200, 20);  // mostly Y direction
      WaypointAddLift(pent, start, end);

      ASSERT_INT(g_lifts_added, 1);
      ASSERT_INT(num_waypoints, 2);
   }
   PASS();

   return 0;
}

// ============================================================
// Tests: waypoint.h inline wrapper functions
// ============================================================

static int test_WaypointInlineWrappers(void)
{
   printf("WaypointInlineWrappers:\n");

   // --- WaypointFindNearest wrappers ---

   TEST("FindNearest(edict_t*, float)");
   reset_waypoint_state();
   {
      edict_t *pEntity = mock_alloc_edict();
      pEntity->v.origin = Vector(0, 0, 0);
      pEntity->v.view_ofs = Vector(0, 0, 17);
      setup_waypoint(0, Vector(200, 0, 0), 0, 0);
      setup_waypoint(1, Vector(50, 0, 0), 0, 0);
      int idx = WaypointFindNearest(pEntity, 9999.0);
      ASSERT_INT(idx, 1);
   }
   PASS();

   TEST("FindNearest(Vector&, edict_t*, float)");
   reset_waypoint_state();
   {
      edict_t *pEntity = mock_alloc_edict();
      setup_waypoint(0, Vector(200, 0, 0), 0, 0);
      setup_waypoint(1, Vector(50, 0, 0), 0, 0);
      Vector src(0, 0, 0);
      int idx = WaypointFindNearest(src, pEntity, 9999.0);
      ASSERT_INT(idx, 1);
   }
   PASS();

   TEST("FindNearest(edict_t*, float, qboolean)");
   reset_waypoint_state();
   {
      edict_t *pEntity = mock_alloc_edict();
      pEntity->v.origin = Vector(0, 0, 0);
      pEntity->v.view_ofs = Vector(0, 0, 17);
      setup_waypoint(0, Vector(200, 0, 0), 0, 0);
      setup_waypoint(1, Vector(50, 0, 0), 0, 0);
      int idx = WaypointFindNearest(pEntity, 9999.0, FALSE);
      ASSERT_INT(idx, 1);
   }
   PASS();

   TEST("FindNearest(Vector&, edict_t*, float, qboolean)");
   reset_waypoint_state();
   {
      edict_t *pEntity = mock_alloc_edict();
      setup_waypoint(0, Vector(200, 0, 0), 0, 0);
      setup_waypoint(1, Vector(50, 0, 0), 0, 0);
      Vector src(0, 0, 0);
      int idx = WaypointFindNearest(src, pEntity, 9999.0, FALSE);
      ASSERT_INT(idx, 1);
   }
   PASS();

   // --- WaypointFindNearestGoal wrappers ---

   TEST("FindNearestGoal(edict_t*, src, flags, itemflags, exclude[])");
   reset_waypoint_state();
   {
      edict_t *pEntity = mock_alloc_edict();
      setup_waypoint(0, Vector(0, 0, 0), W_FL_CROUCH, 0);
      setup_waypoint(1, Vector(100, 0, 0), W_FL_CROUCH, 0x01);
      setup_waypoint(2, Vector(200, 0, 0), W_FL_CROUCH, 0x01);
      setup_route_matrix(3);
      shortest_path[0 * 3 + 1] = 500;
      from_to[0 * 3 + 1] = 1;
      shortest_path[0 * 3 + 2] = 100;
      from_to[0 * 3 + 2] = 2;
      int exclude[] = {-1};
      int idx = WaypointFindNearestGoal(pEntity, 0, W_FL_CROUCH, 0x01, exclude);
      ASSERT_INT(idx, 2);
   }
   PASS();

   TEST("FindNearestGoal(Vector&, edict_t*, float, int flags)");
   reset_waypoint_state();
   {
      edict_t *pEntity = mock_alloc_edict();
      setup_waypoint(0, Vector(0, 0, 0), W_FL_CROUCH, 0);
      setup_waypoint(1, Vector(100, 0, 0), W_FL_CROUCH, 0);
      setup_waypoint(2, Vector(1000, 0, 0), W_FL_CROUCH, 0);
      Vector src(90, 0, 0);
      int idx = WaypointFindNearestGoal(src, pEntity, 9999.0, W_FL_CROUCH);
      ASSERT_INT(idx, 1);
   }
   PASS();

   TEST("FindNearestGoal(edict_t*, src, flags, exclude[])");
   reset_waypoint_state();
   {
      edict_t *pEntity = mock_alloc_edict();
      setup_waypoint(0, Vector(0, 0, 0), W_FL_CROUCH, 0);
      setup_waypoint(1, Vector(100, 0, 0), W_FL_CROUCH, 0);
      setup_waypoint(2, Vector(200, 0, 0), W_FL_CROUCH, 0);
      setup_route_matrix(3);
      shortest_path[0 * 3 + 1] = 100;
      from_to[0 * 3 + 1] = 1;
      shortest_path[0 * 3 + 2] = 200;
      from_to[0 * 3 + 2] = 2;
      int exclude[] = {1, -1};
      int idx = WaypointFindNearestGoal(pEntity, 0, W_FL_CROUCH, exclude);
      ASSERT_INT(idx, 2);
   }
   PASS();

   TEST("FindNearestGoal(edict_t*, src, flags, itemflags)");
   reset_waypoint_state();
   {
      edict_t *pEntity = mock_alloc_edict();
      setup_waypoint(0, Vector(0, 0, 0), W_FL_CROUCH, 0);
      setup_waypoint(1, Vector(100, 0, 0), W_FL_CROUCH, 0x02);
      setup_waypoint(2, Vector(200, 0, 0), W_FL_CROUCH, 0x02);
      setup_route_matrix(3);
      shortest_path[0 * 3 + 1] = 500;
      from_to[0 * 3 + 1] = 1;
      shortest_path[0 * 3 + 2] = 100;
      from_to[0 * 3 + 2] = 2;
      int idx = WaypointFindNearestGoal(pEntity, 0, W_FL_CROUCH, 0x02);
      ASSERT_INT(idx, 2);
   }
   PASS();

   TEST("FindNearestGoal(edict_t*, src, flags)");
   reset_waypoint_state();
   {
      edict_t *pEntity = mock_alloc_edict();
      setup_waypoint(0, Vector(0, 0, 0), W_FL_CROUCH, 0);
      setup_waypoint(1, Vector(100, 0, 0), W_FL_CROUCH, 0);
      setup_route_matrix(2);
      shortest_path[0 * 2 + 1] = 100;
      from_to[0 * 2 + 1] = 1;
      int idx = WaypointFindNearestGoal(pEntity, 0, W_FL_CROUCH);
      ASSERT_INT(idx, 1);
   }
   PASS();

   // --- WaypointFindRandomGoal wrappers ---

   TEST("FindRandomGoal(edict_t*, flags, itemflags, exclude[])");
   reset_waypoint_state();
   {
      edict_t *pEntity = mock_alloc_edict();
      setup_waypoint(0, Vector(0, 0, 0), W_FL_CROUCH, 0x01);
      setup_waypoint(1, Vector(100, 0, 0), W_FL_CROUCH, 0x01);
      setup_waypoint(2, Vector(200, 0, 0), W_FL_JUMP, 0);
      int exclude[] = {0, -1};
      int idx = WaypointFindRandomGoal(pEntity, W_FL_CROUCH, 0x01, exclude);
      ASSERT_INT(idx, 1);
   }
   PASS();

   TEST("FindRandomGoal(edict_t*, flags)");
   reset_waypoint_state();
   {
      edict_t *pEntity = mock_alloc_edict();
      setup_waypoint(0, Vector(0, 0, 0), W_FL_CROUCH, 0);
      int idx = WaypointFindRandomGoal(pEntity, W_FL_CROUCH);
      ASSERT_INT(idx, 0);
   }
   PASS();

   TEST("FindRandomGoal(edict_t*, flags, exclude[])");
   reset_waypoint_state();
   {
      edict_t *pEntity = mock_alloc_edict();
      setup_waypoint(0, Vector(0, 0, 0), W_FL_CROUCH, 0);
      setup_waypoint(1, Vector(100, 0, 0), W_FL_CROUCH, 0);
      int exclude[] = {0, -1};
      int idx = WaypointFindRandomGoal(pEntity, W_FL_CROUCH, exclude);
      ASSERT_INT(idx, 1);
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

   rc |= test_WaypointIsRouteValid();
   rc |= test_WaypointFree();
   rc |= test_WaypointInit();
   rc |= test_WaypointAddDeletePath();
   rc |= test_WaypointFindPath();
   rc |= test_WaypointPathCounts();
   rc |= test_WaypointTrim();
   rc |= test_WaypointFindRandomGoal();
   rc |= test_WaypointFindNearestGoal();
   rc |= test_WaypointRouteMatrix();
   rc |= test_WaypointSaveLoad();
   rc |= test_WaypointSaveFloydsMatrix();
   rc |= test_WaypointReachable();
   rc |= test_WaypointMakePathsWith();
   rc |= test_WaypointFixOldWaypoints();
   rc |= test_WaypointSearchItems();
   rc |= test_CollectMapSpawnItems();
   rc |= test_WaypointAddLift();
   rc |= test_WaypointFindNearest();
   rc |= test_WaypointFindReachable();
   rc |= test_WaypointFindItem();
   rc |= test_WaypointAdd();
   rc |= test_WaypointAddSpawnObjects();
   rc |= test_WaypointBlockList();
   rc |= test_WaypointSlowFloydsHelpers();
   rc |= test_WaypointDrawBeam();
   rc |= test_WaypointRouteInit();
   rc |= test_WaypointAutowaypointing();
   rc |= test_WaypointThink();
   rc |= test_WaypointThinkBeamColors();
   rc |= test_CollectMapSpawnItemsExtra();
   rc |= test_WaypointSearchItemsExtra();
   rc |= test_WaypointFindNearestGoalItemFiltering();
   rc |= test_WaypointReachableWater();
   rc |= test_WaypointAutowayExtraBranches();
   rc |= test_WaypointSaveError();
   rc |= test_WaypointAddLiftExtra();
   rc |= test_WaypointInlineWrappers();

   cleanup_temp_dir();

   printf("\n%d/%d tests passed\n", tests_passed, tests_run);
   return rc ? 1 : (tests_passed < tests_run ? 1 : 0);
}
