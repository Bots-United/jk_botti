//
// JK_Botti - unit tests for posdata_list.h
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/wait.h>
#include <unistd.h>

// Minimal stubs for HL SDK types (no engine deps)
typedef int qboolean;
#define TRUE 1
#define FALSE 0

struct Vector {
   float x, y, z;
};

#include "../posdata_list.h"

static int tests_run = 0;
static int tests_passed = 0;

#define TEST(name) do { \
   tests_run++; \
   printf("  %-50s ", name); \
} while(0)

#define PASS() do { \
   tests_passed++; \
   printf("OK\n"); \
} while(0)

#define ASSERT_PTR_EQ(actual, expected) do { \
   if ((actual) != (expected)) { \
      printf("FAIL\n    expected: %p\n    got:      %p\n", \
             (void*)(expected), (void*)(actual)); \
      return 1; \
   } \
} while(0)

#define ASSERT_PTR_NULL(actual) do { \
   if ((actual) != NULL) { \
      printf("FAIL\n    expected: NULL\n    got:      %p\n", \
             (void*)(actual)); \
      return 1; \
   } \
} while(0)

#define ASSERT_INT(actual, expected) do { \
   if ((actual) != (expected)) { \
      printf("FAIL\n    expected: %d\n    got:      %d\n", \
             (expected), (actual)); \
      return 1; \
   } \
} while(0)

#define ASSERT_TRUE(cond) do { \
   if (!(cond)) { \
      printf("FAIL\n    condition false: %s\n", #cond); \
      return 1; \
   } \
} while(0)

#define TEST_POOL_SIZE 4

static int test_add_single_node(void)
{
   posdata_t pool[TEST_POOL_SIZE];
   posdata_t *latest = NULL, *oldest = NULL;
   memset(pool, 0, sizeof(pool));

   printf("posdata basic operations:\n");

   TEST("add single node: latest and oldest point to it");
   posdata_t *node = posdata_get_slot(pool, TEST_POOL_SIZE, 1.0f);
   ASSERT_TRUE(node != NULL);
   posdata_link_as_latest(&latest, &oldest, node);
   ASSERT_PTR_EQ(latest, node);
   ASSERT_PTR_EQ(oldest, node);
   ASSERT_PTR_NULL(node->older);
   ASSERT_PTR_NULL(node->newer);
   PASS();

   return 0;
}

static int test_add_multiple_nodes(void)
{
   posdata_t pool[TEST_POOL_SIZE];
   posdata_t *latest = NULL, *oldest = NULL;
   memset(pool, 0, sizeof(pool));

   printf("posdata list ordering:\n");

   posdata_t *n0 = posdata_get_slot(pool, TEST_POOL_SIZE, 1.0f);
   posdata_link_as_latest(&latest, &oldest, n0);

   posdata_t *n1 = posdata_get_slot(pool, TEST_POOL_SIZE, 2.0f);
   posdata_link_as_latest(&latest, &oldest, n1);

   posdata_t *n2 = posdata_get_slot(pool, TEST_POOL_SIZE, 3.0f);
   posdata_link_as_latest(&latest, &oldest, n2);

   TEST("oldest is first node added");
   ASSERT_PTR_EQ(oldest, n0);
   PASS();

   TEST("latest is last node added");
   ASSERT_PTR_EQ(latest, n2);
   PASS();

   TEST("forward traversal: oldest->newest via ->newer");
   ASSERT_PTR_EQ(n0->newer, n1);
   ASSERT_PTR_EQ(n1->newer, n2);
   ASSERT_PTR_NULL(n2->newer);
   PASS();

   TEST("backward traversal: newest->oldest via ->older");
   ASSERT_PTR_EQ(n2->older, n1);
   ASSERT_PTR_EQ(n1->older, n0);
   ASSERT_PTR_NULL(n0->older);
   PASS();

   return 0;
}

static int test_timetrim_basic(void)
{
   posdata_t pool[TEST_POOL_SIZE];
   posdata_t *latest = NULL, *oldest = NULL;
   memset(pool, 0, sizeof(pool));

   printf("posdata timetrim:\n");

   // Add 3 nodes at times 1.0, 2.0, 3.0
   posdata_t *n0 = posdata_get_slot(pool, TEST_POOL_SIZE, 1.0f);
   posdata_link_as_latest(&latest, &oldest, n0);

   posdata_t *n1 = posdata_get_slot(pool, TEST_POOL_SIZE, 2.0f);
   posdata_link_as_latest(&latest, &oldest, n1);

   posdata_t *n2 = posdata_get_slot(pool, TEST_POOL_SIZE, 3.0f);
   posdata_link_as_latest(&latest, &oldest, n2);

   // Trim entries with time <= 1.5 (should remove n0 only)
   posdata_timetrim(&latest, &oldest, 1.5f);

   TEST("timetrim removes old entries");
   ASSERT_INT(n0->inuse, FALSE);
   PASS();

   TEST("timetrim preserves new entries");
   ASSERT_INT(n1->inuse, TRUE);
   ASSERT_INT(n2->inuse, TRUE);
   PASS();

   TEST("timetrim updates oldest pointer");
   ASSERT_PTR_EQ(oldest, n1);
   PASS();

   TEST("timetrim: oldest->older is NULL after trim");
   ASSERT_PTR_NULL(n1->older);
   PASS();

   TEST("timetrim: latest unchanged");
   ASSERT_PTR_EQ(latest, n2);
   PASS();

   return 0;
}

static int test_free_list(void)
{
   posdata_t pool[TEST_POOL_SIZE];
   posdata_t *latest = NULL, *oldest = NULL;
   memset(pool, 0, sizeof(pool));

   printf("posdata free_list:\n");

   posdata_t *n0 = posdata_get_slot(pool, TEST_POOL_SIZE, 1.0f);
   posdata_link_as_latest(&latest, &oldest, n0);
   posdata_t *n1 = posdata_get_slot(pool, TEST_POOL_SIZE, 2.0f);
   posdata_link_as_latest(&latest, &oldest, n1);

   posdata_free_list(pool, TEST_POOL_SIZE, &latest, &oldest);

   TEST("free_list: latest is NULL");
   ASSERT_PTR_NULL(latest);
   PASS();

   TEST("free_list: oldest is NULL");
   ASSERT_PTR_NULL(oldest);
   PASS();

   TEST("free_list: all slots cleared");
   int any_inuse = 0;
   for (int i = 0; i < TEST_POOL_SIZE; i++)
      any_inuse |= pool[i].inuse;
   ASSERT_INT(any_inuse, 0);
   PASS();

   return 0;
}

// Regression test: posdata_get_slot stealing an in-use node creates
// a cycle in the linked list (issue #10).
static int test_steal_slot_no_cycle(void)
{
   posdata_t pool[TEST_POOL_SIZE];
   posdata_t *latest = NULL, *oldest = NULL;
   memset(pool, 0, sizeof(pool));

   printf("posdata_get_slot cycle regression (#10):\n");

   // Fill all slots
   for (int i = 0; i < TEST_POOL_SIZE; i++) {
      posdata_t *node = posdata_get_slot(pool, TEST_POOL_SIZE, 1.0f + i);
      posdata_link_as_latest(&latest, &oldest, node);
   }

   // All slots are full. Get another slot — this steals the oldest.
   posdata_t *stolen = posdata_get_slot(pool, TEST_POOL_SIZE, 5.0f);
   posdata_link_as_latest(&latest, &oldest, stolen);

   // Traverse from latest via ->older. If count exceeds pool size, there is a cycle.
   TEST("steal slot: no cycle via ->older traversal");
   int count = 0;
   posdata_t *p = latest;
   while (p) {
      count++;
      if (count > TEST_POOL_SIZE) break;
      p = p->older;
   }
   if (count > TEST_POOL_SIZE) {
      printf("FAIL\n    cycle detected: traversed %d nodes (max %d)\n",
             count, TEST_POOL_SIZE);
      return 1;
   }
   PASS();

   // Also verify the list has the expected number of nodes
   TEST("steal slot: correct node count after steal");
   // After stealing, we should have (pool_size - 1) old nodes + 1 stolen = pool_size
   // But the stolen node was unlinked, so list should have pool_size nodes total
   count = 0;
   p = oldest;
   while (p) {
      count++;
      if (count > TEST_POOL_SIZE) break;
      p = p->newer;
   }
   // With the bug, count may be wrong or infinite. After fix, should be TEST_POOL_SIZE.
   ASSERT_INT(count, TEST_POOL_SIZE);
   PASS();

   return 0;
}

// Regression test: posdata_timetrim dereferences NULL when trimming the
// last remaining node (issue #10, secondary bug).
static int test_timetrim_remove_last_no_crash(void)
{
   printf("posdata_timetrim NULL deref regression (#10):\n");

   TEST("timetrim removing last node: no crash");

   // Run in a subprocess to catch SIGSEGV from the NULL dereference.
   pid_t pid = fork();
   if (pid == 0) {
      posdata_t pool[TEST_POOL_SIZE];
      posdata_t *latest = NULL, *oldest = NULL;
      memset(pool, 0, sizeof(pool));

      // Add one node at time 1.0
      posdata_t *node = posdata_get_slot(pool, TEST_POOL_SIZE, 1.0f);
      posdata_link_as_latest(&latest, &oldest, node);

      // Trim with cutoff >= node's time (removes the only node)
      posdata_timetrim(&latest, &oldest, 1.0f);

      // Verify list is empty
      if (latest != NULL || oldest != NULL)
         _exit(1);

      _exit(0);
   }

   int status;
   waitpid(pid, &status, 0);
   if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
      printf("FAIL\n    child process crashed or returned error (status: 0x%x)\n", status);
      return 1;
   }
   PASS();

   return 0;
}

int main(void)
{
   int rc = 0;

   printf("=== posdata_list tests ===\n\n");

   rc |= test_add_single_node();
   printf("\n");
   rc |= test_add_multiple_nodes();
   printf("\n");
   rc |= test_timetrim_basic();
   printf("\n");
   rc |= test_free_list();
   printf("\n");
   rc |= test_steal_slot_no_cycle();
   printf("\n");
   rc |= test_timetrim_remove_last_no_crash();

   printf("\n%d/%d tests passed.\n", tests_passed, tests_run);

   if (rc || tests_passed != tests_run) {
      printf("SOME TESTS FAILED!\n");
      return 1;
   }

   printf("All tests passed.\n");
   return 0;
}
