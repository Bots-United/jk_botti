//
// JK_Botti - tests for neuralnet and genetic algorithm
//

#include <stdlib.h>
#include <math.h>

#include "test_common.h"

#include "neuralnet.h"
#include "geneticalg.h"

extern void fast_random_seed(unsigned int seed);
extern float RANDOM_FLOAT2(float flLow, float flHigh);

// return random value in range 0 <= n <= 1
static double get_random(void)
{
   return RANDOM_FLOAT2(0.0, 1.0);
}

// ============================================================
// Unit tests
// ============================================================

static int test_neuralnet_construction(void)
{
   printf("neuralnet construction:\n");

   TEST("create 1-input 1-output network");
   CNeuralNet *nnet = new CNeuralNet(1, 1, 1, 4);
   ASSERT_INT(nnet->get_num_inputs(), 1);
   ASSERT_INT(nnet->get_num_outputs(), 1);
   ASSERT_TRUE(nnet->get_num_weights() > 0);
   PASS();

   TEST("run produces output");
   double input[1] = {0.5};
   double output[1] = {0.0};
   ASSERT_PTR_NOT_NULL(nnet->run(input, output));
   PASS();

   TEST("run with scale produces output");
   double scale[1] = {10.0};
   double output2[1] = {0.0};
   ASSERT_PTR_NOT_NULL(nnet->run(input, output2, scale));
   PASS();

   TEST("weights round-trip");
   int nw = nnet->get_num_weights();
   double *w1 = (double *)malloc(nw * sizeof(double));
   double *w2 = (double *)malloc(nw * sizeof(double));
   nnet->get_weights(w1);
   nnet->put_weights(w1);
   nnet->get_weights(w2);
   bool match = true;
   for (int i = 0; i < nw; i++)
      if (w1[i] != w2[i]) { match = false; break; }
   free(w1);
   free(w2);
   ASSERT_TRUE(match);
   PASS();

   delete nnet;
   return 0;
}

static int test_population(void)
{
   printf("population:\n");

   TEST("create population");
   CPopulation pop(10, 5);
   ASSERT_INT(pop.get_size(), 10);
   ASSERT_INT(pop.get_genome_length(), 5);
   PASS();

   TEST("get individual");
   CGenome *g = pop.get_individual(0);
   ASSERT_PTR_NOT_NULL(g);
   ASSERT_INT(g->length(), 5);
   PASS();

   TEST("get fittest individual");
   // set known fitness values
   pop.get_individual(3)->m_fitness = 100.0;
   CGenome *fittest = pop.get_fittest_individual();
   ASSERT_PTR_EQ(fittest, pop.get_individual(3));
   PASS();

   TEST("swap populations");
   CPopulation pop2(10, 5);
   double gene0_a = pop.get_individual(0)->m_genes[0];
   double gene0_b = pop2.get_individual(0)->m_genes[0];
   pop.swap(pop2);
   ASSERT_TRUE(pop.get_individual(0)->m_genes[0] == gene0_b);
   ASSERT_TRUE(pop2.get_individual(0)->m_genes[0] == gene0_a);
   PASS();

   return 0;
}

// ============================================================
// Training test: approximate f(x) = x^2 via genetic algorithm
// ============================================================

static double target_func(double x)
{
   return x * x;
}

static int test_training(void)
{
   int i, j, k;
   double input[3];
   double output[1];
   double scale[1] = {100};
   double diff;

   printf("neuralnet training (f(x) = x^2):\n");

   // fixed seed for reproducibility
   fast_random_seed(42);

   CNeuralNet *nnet = new CNeuralNet(1, 1, 1, 4);
   CPopulation population(25, nnet->get_num_weights());
   CGeneticAlgorithm genalg(0.2, 0.7);

   for (k = 0; k < 1000; k++) {
      for (j = 0; j < population.get_size(); j++) {
         double x, foutput;
         CGenome *genome = population.get_individual(j);

         nnet->put_weights(genome->m_genes);

         for (x = -0.5; x <= 4.5; x += 1) {
            input[0] = x;
            nnet->run(input, output, scale);

            foutput = target_func(input[0]);
            diff = output[0] - foutput;
            genome->m_fitness += -(diff * diff) * 0.0002;
         }

         for (i = 0; i < 500; i++) {
            input[0] = get_random() * 5 - 0.5;
            nnet->run(input, output, scale);

            foutput = target_func(input[0]);
            diff = output[0] - foutput;
            genome->m_fitness += -(diff * diff) * 0.0001;
         }
      }

      if (k + 1 < 1000) {
         CPopulation new_pop(population.get_size(), nnet->get_num_weights());
         genalg.epoch(population, new_pop);
         population.swap(new_pop);
      }
   }

   CGenome *best = population.get_fittest_individual();
   nnet->put_weights(best->m_genes);

   TEST("fitness improved from initial");
   ASSERT_TRUE(best->m_fitness > -1.0);
   PASS();

   // check trained network approximates x^2 at several points
   double max_err = 0;
   for (i = 0; i < 9; i++) {
      input[0] = i * 0.5;
      nnet->run(input, output, scale);
      double expected = target_func(input[0]);
      double err = fabs(output[0] - expected);
      if (err > max_err)
         max_err = err;
   }

   TEST("approximation error < 2.0 across test points");
   if (max_err >= 2.0)
      printf("(max_err=%.3f) ", max_err);
   ASSERT_TRUE(max_err < 2.0);
   PASS();

   delete nnet;
   return 0;
}

int main(void)
{
   int rc = 0;

   printf("=== neuralnet tests ===\n\n");

   rc |= test_neuralnet_construction();
   printf("\n");
   rc |= test_population();
   printf("\n");
   rc |= test_training();

   printf("\n%d/%d tests passed.\n", tests_passed, tests_run);

   if (rc || tests_passed != tests_run) {
      printf("SOME TESTS FAILED!\n");
      return 1;
   }

   printf("All tests passed.\n");
   return 0;
}
