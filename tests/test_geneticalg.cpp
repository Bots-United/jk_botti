//
// JK_Botti - unit tests for geneticalg.cpp
//

#include <stdlib.h>
#include <math.h>
#include <limits.h>

#include "test_common.h"

// Make private members accessible for testing
#define private public
#define protected public
#include "../geneticalg.cpp"
#undef private
#undef protected

// ============================================================
// test_CGenome
// ============================================================

static int test_CGenome(void)
{
   printf("CGenome:\n");

   TEST("default constructor");
   {
      CGenome g;
      ASSERT_TRUE(g.m_fitness == -9999999999.9);
      ASSERT_INT(g.length(), 0);
      ASSERT_PTR_NULL(g.m_genes);
   }
   PASS();

   TEST("copy_from copies genes and fitness");
   {
      double src_genes[3] = {1.0, 2.0, 3.0};
      double dst_genes[3] = {0.0, 0.0, 0.0};
      CGenome src(src_genes, 3, 42.0);
      CGenome dst(dst_genes, 3, 0.0);

      dst.copy_from(src);

      ASSERT_TRUE(dst.m_fitness == 42.0);
      ASSERT_TRUE(dst.m_genes[0] == 1.0);
      ASSERT_TRUE(dst.m_genes[1] == 2.0);
      ASSERT_TRUE(dst.m_genes[2] == 3.0);

      // modifying copy doesn't affect source
      dst.m_genes[0] = 99.0;
      dst.m_fitness = -1.0;
      ASSERT_TRUE(src.m_genes[0] == 1.0);
      ASSERT_TRUE(src.m_fitness == 42.0);
   }
   PASS();

   return 0;
}

// ============================================================
// test_CPopulation
// ============================================================

static int test_CPopulation(void)
{
   printf("CPopulation:\n");

   fast_random_seed(42);

   TEST("constructor with zero pop_size");
   {
      CPopulation pop(0, 5);
      ASSERT_INT(pop.get_size(), 0);
      ASSERT_PTR_NULL(pop.get_fittest_individual());
   }
   PASS();

   TEST("constructor with zero genome_length");
   {
      CPopulation pop(5, 0);
      ASSERT_INT(pop.m_pool_size, 0);
   }
   PASS();

   TEST("constructor integer overflow guard");
   {
      CPopulation pop(INT_MAX, INT_MAX);
      ASSERT_INT(pop.m_pool_size, 0);
   }
   PASS();

   TEST("free_mem idempotent");
   {
      CPopulation pop(4, 3);
      pop.free_mem();
      pop.free_mem();
      // no crash = pass
   }
   PASS();

   TEST("get_fittest_individual on default-constructed pop");
   {
      CPopulation pop;
      ASSERT_PTR_NULL(pop.get_fittest_individual());
   }
   PASS();

   return 0;
}

// ============================================================
// test_fitness_sort
// ============================================================

static int test_fitness_sort_func(void)
{
   printf("fitness_sort:\n");

   TEST("sorts ascending by fitness");
   {
      double genes_a[1] = {0.0};
      double genes_b[1] = {0.0};
      double genes_c[1] = {0.0};
      double genes_d[1] = {0.0};

      CGenome arr[4];
      arr[0] = CGenome(genes_a, 1, 3.0);
      arr[1] = CGenome(genes_b, 1, 1.0);
      arr[2] = CGenome(genes_c, 1, 4.0);
      arr[3] = CGenome(genes_d, 1, 2.0);

      qsort(arr, 4, sizeof(CGenome), fitness_sort);

      ASSERT_TRUE(arr[0].m_fitness == 1.0);
      ASSERT_TRUE(arr[1].m_fitness == 2.0);
      ASSERT_TRUE(arr[2].m_fitness == 3.0);
      ASSERT_TRUE(arr[3].m_fitness == 4.0);
   }
   PASS();

   return 0;
}

// ============================================================
// test_CGeneticAlgorithm
// ============================================================

static int test_CGeneticAlgorithm(void)
{
   printf("CGeneticAlgorithm:\n");

   fast_random_seed(42);

   TEST("epoch rejects mismatched pop sizes");
   {
      CPopulation old_pop(10, 5);
      CPopulation new_pop(8, 5);
      CGeneticAlgorithm ga(0.2, 0.7);
      ASSERT_PTR_NULL(ga.epoch(old_pop, new_pop));
   }
   PASS();

   TEST("epoch rejects mismatched genome lengths");
   {
      CPopulation old_pop(10, 5);
      CPopulation new_pop(10, 3);
      CGeneticAlgorithm ga(0.2, 0.7);
      ASSERT_PTR_NULL(ga.epoch(old_pop, new_pop));
   }
   PASS();

   TEST("epoch preserves elite genes");
   {
      fast_random_seed(42);
      CPopulation old_pop(10, 5);
      CPopulation new_pop(10, 5);
      CGeneticAlgorithm ga(0.2, 0.7);

      // set distinct fitness values so we know the best
      for (int i = 0; i < old_pop.get_size(); i++)
         old_pop.get_individual(i)->m_fitness = (double)(i + 1);

      // best individual has fitness 10.0 (index 9)
      CGenome *best = old_pop.get_individual(9);
      double best_genes[5];
      for (int i = 0; i < 5; i++)
         best_genes[i] = best->m_genes[i];

      ga.epoch(old_pop, new_pop);

      // elite should appear in new_pop (genes match)
      bool found = false;
      for (int i = 0; i < new_pop.get_size(); i++) {
         CGenome *g = new_pop.get_individual(i);
         bool match = true;
         for (int j = 0; j < 5; j++) {
            if (g->m_genes[j] != best_genes[j]) {
               match = false;
               break;
            }
         }
         if (match) { found = true; break; }
      }
      ASSERT_TRUE(found);
   }
   PASS();

   TEST("epoch with odd population count");
   {
      fast_random_seed(42);
      CPopulation old_pop(11, 5);
      CPopulation new_pop(11, 5);
      CGeneticAlgorithm ga(0.2, 0.7);

      for (int i = 0; i < old_pop.get_size(); i++)
         old_pop.get_individual(i)->m_fitness = (double)i;

      CPopulation *result = ga.epoch(old_pop, new_pop);
      ASSERT_PTR_NOT_NULL(result);
   }
   PASS();

   TEST("crossover with same parent copies");
   {
      fast_random_seed(42);
      double p_genes[4] = {1.0, 2.0, 3.0, 4.0};
      double c1_genes[4] = {0.0, 0.0, 0.0, 0.0};
      double c2_genes[4] = {0.0, 0.0, 0.0, 0.0};
      CGenome parent(p_genes, 4, 5.0);
      CGenome child1(c1_genes, 4, 0.0);
      CGenome child2(c2_genes, 4, 0.0);

      CPopulation pop(4, 4);
      CGeneticAlgorithm ga(0.2, 0.7);
      ga.m_population = &pop;

      ga.crossover(parent, parent, child1, child2);

      for (int i = 0; i < 4; i++) {
         ASSERT_TRUE(child1.m_genes[i] == p_genes[i]);
         ASSERT_TRUE(child2.m_genes[i] == p_genes[i]);
      }
   }
   PASS();

   TEST("crossover with rate 0 copies parents unchanged");
   {
      fast_random_seed(42);
      double p1_genes[4] = {1.0, 2.0, 3.0, 4.0};
      double p2_genes[4] = {5.0, 6.0, 7.0, 8.0};
      double c1_genes[4] = {0.0, 0.0, 0.0, 0.0};
      double c2_genes[4] = {0.0, 0.0, 0.0, 0.0};
      CGenome parent1(p1_genes, 4, 1.0);
      CGenome parent2(p2_genes, 4, 2.0);
      CGenome child1(c1_genes, 4, 0.0);
      CGenome child2(c2_genes, 4, 0.0);

      CPopulation pop(4, 4);
      CGeneticAlgorithm ga(0.2, 0.0);  // crossover_rate = 0
      ga.m_population = &pop;

      ga.crossover(parent1, parent2, child1, child2);

      // get_random() > 0.0 is almost always true, so children copy parents
      for (int i = 0; i < 4; i++) {
         ASSERT_TRUE(child1.m_genes[i] == p1_genes[i]);
         ASSERT_TRUE(child2.m_genes[i] == p2_genes[i]);
      }
   }
   PASS();

   TEST("calculate_fitness_values computes correctly");
   {
      fast_random_seed(42);
      CPopulation pop(4, 2);
      CGeneticAlgorithm ga(0.2, 0.7);
      ga.m_population = &pop;

      pop.get_individual(0)->m_fitness = 1.0;
      pop.get_individual(1)->m_fitness = 5.0;
      pop.get_individual(2)->m_fitness = 3.0;
      pop.get_individual(3)->m_fitness = -2.0;

      ga.calculate_fitness_values();

      ASSERT_TRUE(ga.m_best_fitness == 5.0);
      ASSERT_TRUE(ga.m_worst_fitness == -2.0);
      ASSERT_TRUE(ga.m_total_fitness == 7.0);  // 1+5+3+(-2)
      ASSERT_TRUE(ga.m_average_fitness == 7.0 / 4.0);
      ASSERT_INT(ga.m_fittest_individual, 1);
   }
   PASS();

   TEST("get_individual_random_weighted all-zero fitness");
   {
      fast_random_seed(42);
      CPopulation pop(4, 2);
      CGeneticAlgorithm ga(0.2, 0.7);
      ga.m_population = &pop;

      for (int i = 0; i < pop.get_size(); i++)
         pop.get_individual(i)->m_fitness = 0.0;

      ga.m_total_fitness = 0.0;
      ga.m_worst_fitness = 0.0;

      CGenome *result = ga.get_individual_random_weighted();
      ASSERT_PTR_NOT_NULL(result);
   }
   PASS();

   TEST("get_crossover_interleave clamping");
   {
      // interleave > num_genes returns num_genes
      CGeneticAlgorithm ga_high(0.2, 0.7, 100);
      ASSERT_INT(ga_high.get_crossover_interleave(5), 5);

      // interleave < 1 returns random in [1, num_genes]
      fast_random_seed(42);
      CGeneticAlgorithm ga_low(0.2, 0.7, 0);
      int val = ga_low.get_crossover_interleave(5);
      ASSERT_TRUE(val >= 1 && val <= 5);
   }
   PASS();

   return 0;
}

// ============================================================
// main
// ============================================================

int main(void)
{
   int rc = 0;

   printf("=== geneticalg tests ===\n\n");

   rc |= test_CGenome();
   printf("\n");
   rc |= test_CPopulation();
   printf("\n");
   rc |= test_fitness_sort_func();
   printf("\n");
   rc |= test_CGeneticAlgorithm();

   printf("\n%d/%d tests passed.\n", tests_passed, tests_run);

   if (rc || tests_passed != tests_run) {
      printf("SOME TESTS FAILED!\n");
      return 1;
   }

   printf("All tests passed.\n");
   return 0;
}
