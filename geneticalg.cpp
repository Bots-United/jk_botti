#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <memory.h>
#include <math.h>
#include <limits.h>

#include "geneticalg.h"

extern void fast_random_seed(unsigned int seed);
extern int RANDOM_LONG2(int lLow, int lHigh);
extern float RANDOM_FLOAT2(float flLow, float flHigh);

static int get_random_int(int x,int y)
{
   return RANDOM_LONG2(x, y);
}

// return random value in range 0 < n < 1
static double get_random(void)
{
   return RANDOM_FLOAT2(0.0, 1.0);
}

// return random value in range -1 < n < 1
static double get_random_weight(void)
{
   return get_random() - get_random();
}

/******************************************************************* CGenome */
void CGenome::copy_from(const CGenome &from)
{
   int i;

   for (i = 0; i < m_genome_length; i++)
      m_genes[i] = from.m_genes[i];

   m_fitness = from.m_fitness;
}

/*************************************************************** CPopulation */
CPopulation::CPopulation(int population_size, int genome_length):
      m_pop_size(population_size),
      m_individuals(NULL),
      m_genome_length(genome_length),
      m_pool_size(0),
      m_genepool(NULL)
{
   int i;

   // check for integer overflow in pool_size calculation
   if (population_size <= 0 || genome_length <= 0 ||
       population_size > INT_MAX / genome_length)
      return;

   m_pool_size = population_size * genome_length;

   m_individuals = new CGenome[m_pop_size];
   m_genepool = (double *)calloc(1, sizeof(double) * m_pool_size);
   if (!m_genepool)
      return;

   for (i = 0; i < m_pop_size; i++)
      m_individuals[i] = CGenome(&m_genepool[i * m_genome_length], m_genome_length, 0.0);

   for (i = 0; i < m_pool_size; i++)
      m_genepool[i] = get_random_weight();
}

void CPopulation::free_mem(void)
{
   if (m_individuals) {
      delete[] m_individuals;
      m_individuals = NULL;
   }

   if (m_genepool) {
      free(m_genepool);
      m_genepool = NULL;
   }
}

void CPopulation::swap(CPopulation &other)
{
   CPopulation tmp;

   // move this -> tmp
   tmp.m_pop_size = m_pop_size;
   tmp.m_individuals = m_individuals;
   tmp.m_genome_length = m_genome_length;
   tmp.m_pool_size = m_pool_size;
   tmp.m_genepool = m_genepool;

   // move other -> this
   m_pop_size = other.m_pop_size;
   m_individuals = other.m_individuals;
   m_genome_length = other.m_genome_length;
   m_pool_size = other.m_pool_size;
   m_genepool = other.m_genepool;

   // move tmp -> other
   other.m_pop_size = tmp.m_pop_size;
   other.m_individuals = tmp.m_individuals;
   other.m_genome_length = tmp.m_genome_length;
   other.m_pool_size = tmp.m_pool_size;
   other.m_genepool = tmp.m_genepool;

   // null out tmp so its destructor doesn't free anything
   tmp.m_individuals = NULL;
   tmp.m_genepool = NULL;
}

CGenome *CPopulation::get_fittest_individual(void)
{
   double highest;
   int highest_idx;
   int i;

   if (m_pop_size == 0)
      return NULL;

   highest = m_individuals[0].m_fitness;
   highest_idx = 0;

   for (i = 1; i < m_pop_size; i++) {
      if (highest < m_individuals[i].m_fitness) {
         highest = m_individuals[i].m_fitness;
         highest_idx = i;
      }
   }

   return &m_individuals[highest_idx];
}

/********************************************************* CGeneticAlgorithm */
int CGeneticAlgorithm::get_crossover_interleave(int num_genes)
{
   if (m_crossover_interleave > num_genes)
      return num_genes;

   if (m_crossover_interleave < 1)
      return get_random_int(1, num_genes + 1);

   return m_crossover_interleave;
}

void CGeneticAlgorithm::crossover(CGenome &parent1, CGenome &parent2, CGenome &child1, CGenome &child2)
{
   int num_genes = parent1.length();
   int i, j;

   if (&parent1 == &parent2 || get_random() > m_crossover_rate) {
      child1.copy_from(parent1);
      child2.copy_from(parent2);
      return;
   }

   // split genome into chromosomes
   int gene_pos = 0;
   int num_chromos = get_crossover_interleave(num_genes);
   int num_genes_per_chromo = num_genes / num_chromos;
   int num_genes_last_chromo = num_genes - num_chromos * num_genes_per_chromo;

   for (i = 0; i < num_chromos; i++, gene_pos += num_genes_per_chromo) {
      int cp = get_random_int(0, num_genes_per_chromo - 1);

      // create new chromosome
      for (j = 0; j < cp; j++) {
         child1.m_genes[gene_pos + j] = parent1.m_genes[gene_pos + j];
         child2.m_genes[gene_pos + j] = parent2.m_genes[gene_pos + j];
      }

      for (j = cp; j < num_genes_per_chromo; j++)
      {
         child1.m_genes[gene_pos + j] = parent2.m_genes[gene_pos + j];
         child2.m_genes[gene_pos + j] = parent1.m_genes[gene_pos + j];
      }
   }

   if (num_genes_last_chromo > 0) {
      int cp = get_random_int(0, num_genes_last_chromo - 1);

      // create new chromosome
      for (j = 0; j < cp; j++) {
         child1.m_genes[gene_pos + j] = parent1.m_genes[gene_pos + j];
         child2.m_genes[gene_pos + j] = parent2.m_genes[gene_pos + j];
      }

      for (j = cp; j < num_genes_last_chromo; j++)
      {
         child1.m_genes[gene_pos + j] = parent2.m_genes[gene_pos + j];
         child2.m_genes[gene_pos + j] = parent1.m_genes[gene_pos + j];
      }
   }

   /*
   // use fitness of parent which has it lowest
   child1.m_fitness = parent1.m_fitness < parent2.m_fitness ? parent1.m_fitness : parent2.m_fitness;
   child2.m_fitness = parent1.m_fitness < parent2.m_fitness ? parent1.m_fitness : parent2.m_fitness;
   */
   child1.m_fitness = 0.0;
   child2.m_fitness = 0.0;
}

void CGeneticAlgorithm::mutate(CGenome &individual)
{
   int num_genes = m_population->get_genome_length();
   int i;

   // traverse the chromosome and mutate each weight dependent on the mutation rate
   for (i = 0; i < num_genes; i++)  {
      //do we perturb this gene
      if (get_random() < m_mutation_rate) {
         //add or subtract a small value to the gene
      // individual.m_genes[i] += get_random_weight() * m_max_perturbation;

         if (fabs(individual.m_genes[i]) > 1.0)
            individual.m_genes[i] += get_random_weight() * fabs(individual.m_genes[i]) * m_max_perturbation;
         else
            individual.m_genes[i] += get_random_weight() * m_max_perturbation;
      }
   }
}

CGenome *CGeneticAlgorithm::get_individual_random_weighted(void)
{
   int i;
   double fitness_pos = 0.0;

   // offset fitness so worst individual has effective fitness 0
   // (roulette wheel selection requires non-negative values)
   double offset = m_worst_fitness < 0 ? -m_worst_fitness : 0;
   double total = m_total_fitness + offset * m_population->get_size();

   // if all individuals have equal fitness, fall back to uniform random
   if (total <= 0.0)
      return get_individual_random();

   double slice = get_random() * total;

   for (i = 0; i < m_population->get_size(); i++) {
      fitness_pos += m_population->get_fitness_of(i) + offset;

      if (fitness_pos >= slice)
         return m_population->get_individual(i);
   }

   return m_population->get_individual(m_population->get_size() - 1);
}

CGenome *CGeneticAlgorithm::get_individual_random(void)
{
   return m_population->get_individual(get_random_int(0, m_population->get_size()-1));
}

int CGeneticAlgorithm::grab_num_best(const int num_best, const int num_copies, CPopulation &population, int pop_pos)
{
   int i, j, count = 0;
   CGenome *elite;
   CGenome *target;

   // add the required amount of copies of the n most fittest to the supplied vector
   for (i = 0; i < num_best && i < m_population->get_size(); i++) {
      for (j = 0; j < num_copies; j++) {
         int eidx = m_population->get_size() - 1 - i;
         int tidx = pop_pos + count;

         if (tidx >= population.get_size())
            return population.get_size();

         elite = m_population->get_individual(eidx);
         target = population.get_individual(tidx);

         target->copy_from(*elite);
         target->m_fitness = 0.0;
         count++;
      }
   }

   return pop_pos + count;
}

int CGeneticAlgorithm::add_random_genome(CPopulation &population, int pop_pos)
{
   int i, j, count = 0;
   CGenome *target;

   for (i = 0; i < m_num_new_random; i++) {
      if (pop_pos + count >= population.get_size())
         return population.get_size();

      target = population.get_individual(pop_pos + count);

      for (j = 0; j < target->length(); j++)
          target->m_genes[j] = get_random_weight();

      count++;
   }

   return pop_pos + count;
}

void CGeneticAlgorithm::calculate_fitness_values(void)
{
   int i;
   double highest;
   double lowest;

   m_total_fitness = 0;

   highest = m_population->get_fitness_of(0);
   m_fittest_individual = 0;
   m_best_fitness = highest;

   lowest = m_population->get_fitness_of(0);
   m_worst_fitness = lowest;

   m_total_fitness = m_population->get_fitness_of(0);

   for (i = 1; i < m_population->get_size(); i++) {
      // update fittest if necessary
      if (m_population->get_fitness_of(i) > highest) {
         highest = m_population->get_fitness_of(i);
         m_fittest_individual = i;
         m_best_fitness = highest;
      }

      // update worst if necessary
      if (m_population->get_fitness_of(i) < lowest) {
         lowest = m_population->get_fitness_of(i);
         m_worst_fitness = lowest;
      }
      m_total_fitness += m_population->get_fitness_of(i);
   }

   m_average_fitness = m_total_fitness / m_population->get_size();
}

void CGeneticAlgorithm::reset_fitness_values(void)
{
   m_total_fitness = 0;
   m_best_fitness = 0;
   m_worst_fitness = 9999999999.9;
   m_average_fitness = 0;
}

static int fitness_sort(const void *va, const void *vb)
{
   const CGenome *a = (const CGenome *)va;
   const CGenome *b = (const CGenome *)vb;
   return (a->m_fitness > b->m_fitness) - (a->m_fitness < b->m_fitness);
}

CPopulation *CGeneticAlgorithm::epoch(CPopulation &old_pop, CPopulation &new_pop)
{
   int i, new_pos = 0;
   double *tmp_child_mem = NULL;
   CGenome tmp_child;

   if (old_pop.get_size() != new_pop.get_size()) {
      printf("CGeneticAlgorithm::epoch(): ERROR, different size populations\n");
      return NULL;
   }

   if (old_pop.get_genome_length() != new_pop.get_genome_length()) {
      printf("CGeneticAlgorithm::epoch(): ERROR, different size genomes\n");
      return NULL;
   }

   m_generation++;

   // assign the given population to the classes population
   m_population = &old_pop;

   // reset the appropriate variables
   reset_fitness_values();

   // sort the population (for scaling and elitism)
   qsort(m_population->get_individual(0), m_population->get_size(), sizeof(CGenome), fitness_sort);

   // calculate best, worst, average and total fitness
   calculate_fitness_values();

/*
   printf(" best: %f\n", m_best_fitness);
   printf("  avg: %f\n", m_average_fitness);
   printf("worst: %f\n", m_worst_fitness);
   printf("total: %f\n", m_total_fitness);

   for (i = 0; i < old_pop.get_size(); i++)
      printf(" fitness[%i]: %f\n", i, old_pop.get_fitness_of(i));
*/

   // now to add a little elitism we shall add in some copies of the fittest genomes.
   new_pos = grab_num_best(m_num_elite, m_num_copies_elite, new_pop, new_pos);

   // add some completely new creatures to generate new genes
   new_pos = add_random_genome(new_pop, new_pos);

   // repeat until a new population is generated
   for (i = new_pos; i < new_pop.get_size(); i++)
   {
      CGenome *parent1, *parent2, *child1, *child2;

      parent1 = get_individual_random();
      parent2 = get_individual_random_weighted();

      // get target genomes
      child1 = new_pop.get_individual(i);
      if (++i >= new_pop.get_size()) {
         // use temporary
         if (!tmp_child_mem)
            tmp_child_mem = (double *)malloc(sizeof(double) * parent1->length());
         if (!tmp_child_mem)
            break;

         tmp_child = CGenome(tmp_child_mem, parent1->length(), 0.0);
         child2 = &tmp_child;
      } else
         child2 = new_pop.get_individual(i);

      crossover(*parent1, *parent2, *child1, *child2);

      mutate(*child1);
      mutate(*child2);

      child1->m_fitness = 0.0;
      child2->m_fitness = 0.0;
   }

   m_population = &new_pop;

   if (tmp_child_mem)
      free(tmp_child_mem);

   // sort the population (for scaling and elitism)
   qsort(m_population->get_individual(0), m_population->get_size(), sizeof(CGenome), fitness_sort);

   return m_population;
}
