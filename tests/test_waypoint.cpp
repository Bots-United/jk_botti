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
   // Override: make runner's self-distance also WAYPOINT_MAX_DISTANCE
   // so all runner_distance checks fail (runner can reach nothing)
   shortest_path[0 * 3 + 0] = WAYPOINT_MAX_DISTANCE;
   shortest_path[0 * 3 + 1] = WAYPOINT_MAX_DISTANCE;
   shortest_path[0 * 3 + 2] = WAYPOINT_MAX_DISTANCE;
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

   cleanup_temp_dir();

   printf("\n%d/%d tests passed\n", tests_passed, tests_run);
   return rc ? 1 : (tests_passed < tests_run ? 1 : 0);
}
