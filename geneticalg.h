#include "new_baseclass.h"

/******************************************************************* CGenome */
class CGenome : public class_new_baseclass {
public:
	int m_genome_length;
	double *m_genes;

	double m_fitness;

	CGenome(void): m_genome_length(0), m_genes(NULL), m_fitness(-9999999999.9) {}
	CGenome(double in_genes[], int genome_length, double fitness): m_genome_length(genome_length), m_genes(in_genes), m_fitness(fitness) {}

	void copy_from(const CGenome &from);

	int length(void) const { return m_genome_length; }
};

/*************************************************************** CPopulation */
class CPopulation : public class_new_baseclass {
private:
	int m_pop_size;
	CGenome *m_individuals;

	int m_genome_length;

	int m_pool_size;
	double *m_genepool;

public:
	CPopulation(void): m_pop_size(0), m_individuals(NULL), m_genome_length(0), m_pool_size(0), m_genepool(NULL) {}
	CPopulation(int population_size, int genome_length);

	void free_mem(void);

	int get_size(void) const { return m_pop_size; }
	int get_genome_length(void) const { return m_genome_length; }
	double get_fitness_of(int i) const { return m_individuals[i].m_fitness; }
	CGenome *get_individual(int i) { return &m_individuals[i]; }
	CGenome *get_fittest_individual(void);
};

/********************************************************* CGeneticAlgorithm */
class CGeneticAlgorithm : public class_new_baseclass {
private:
	// population
	CPopulation *m_population;

	// total, best, average and worst fitness of population
	double m_total_fitness;
	double m_best_fitness;
	double m_average_fitness;
	double m_worst_fitness;

	// keeps track of the best genome
	int m_fittest_individual;

	int m_num_elite;
	int m_num_copies_elite;

	int m_num_new_random;

	// probability that a gene bits will mutate (~0.05 - 0.3)
	double m_mutation_rate;
	double m_max_perturbation;

	// probability of chromosones crossing over bits (~0.7)
	double m_crossover_rate;

	// use interleaving for genome? 1 = one chromosome, 2 = two chromosomes, etc per genome
	int m_crossover_interleave;

	// generation counter
	int m_generation;

	void crossover(CGenome &parent1, CGenome &parent2, CGenome &child1, CGenome &child2);
	void mutate(CGenome &individual);
	CGenome *get_individual_random_weighted(void);
	CGenome *get_individual_random(void);

	// use to introduce elitism
	int grab_num_best(const int num_best, const int num_copies, CPopulation &population, int pop_pos);

	// use to introduce randomness
	int add_random_genome(CPopulation &population, int pop_pos);

	void calculate_fitness_values(void);
	void reset_fitness_values(void);

	int get_crossover_interleave(int num_genes);

	static int fitness_sort(const CGenome *a, const CGenome *b);

public:
	CGeneticAlgorithm(double mutation_rate, double crossover_rate, int crossover_interleave):
		m_population(NULL),
		m_total_fitness(0),
		m_best_fitness(0),
		m_average_fitness(0),
		m_worst_fitness(9999999999.9),
		m_fittest_individual(0),
		m_num_elite(1),
		m_num_copies_elite(2),
		m_num_new_random(4),
		m_mutation_rate(mutation_rate),
		m_max_perturbation(0.3),
		m_crossover_rate(crossover_rate),
		m_crossover_interleave(crossover_interleave),
		m_generation(1)
	{
	}

	CPopulation *epoch(CPopulation &old_population, CPopulation &new_population);

	CPopulation *get_population(void) { return m_population; }

	int get_generation() const { return m_generation; }
};

