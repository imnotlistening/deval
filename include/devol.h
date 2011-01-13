
/*
 * Data structure layouts. This stuff is important.
 */

#ifndef _DEVOL_H

#include <devol_threads.h>

/* Function return codes. */
#define DEVOL_ERR  -1
#define DEVOL_OK   0

/* Simple debugging/info messages switchable in the build process. */
# ifdef _DEBUG
#  define DEBUG(...) printf(__VA_ARGS__)
# else
#  define DEBUG(...)
# endif

# ifdef _INFO
#  define INFO(...) printf(__VA_ARGS__)
# else
#  define INFO(...)
# endif

/*
 * A struct for a solution. This holds all of an individuals solution data.
 * The call back functions are defined by another program.
 */
struct solution {

  /* These are the functions that will modify the solution. */
  int    (*mutate)(struct solution *par1, struct solution *par2, 
		   struct solution *dest);
  double (*fitness)(struct solution *solution);
  int    (*init)(struct solution *solution);
  int    (*destroy)(struct solution *solution);

  /* The calculated fitness value. */
  double fitness_val;

  /* The solution's private data. */
  void *private;

};

typedef struct solution solution_t;

/*
 * A struct for defining and passing various paramaters for the genetic
 * algorithm.
 */
struct devol_params {

  /* Pass in the call back functions that will be propagated to each individual
   * solution. */
  int    (*mutate)(solution_t *par1, solution_t *par2, solution_t *dest);
  double (*fitness)(solution_t *solution);
  int    (*init)(solution_t *solution);
  int    (*destroy)(solution_t *solution);

  /* How much gene dispersal do we want? 0 is no dispersal. */
  double gene_dispersal_factor;

  /* How many new solutions should we breed. Varies between 0 and 1: 0 being
   * no new solutions (not a very good algorithm), and 1 being 1 new solution
   * for each member in the population. */
  double reproduction_rate;

  /* Specify what percent of the population is OK as a breeder. For instance
   * a value of .07 would say that the top 7% of the population is acceptable.
   * Thus the smaller this number, the more picky we are about who is fit
   * enough to breed. */
  double breed_fitness;

};

/*
 * This struct defines a pool of solutions. The call back function pointers
 * are used to initialize and manipulate a population of solutions.
 */
struct gene_pool {

  /* The soltuions themselves. */
  solution_t *solutions;
  size_t      solution_count;

  /* A pool of threads to use for distributing the work. */
  struct thread_pool workers;

  /* An aggragation of the myriad paramaters that go into an evolutionary
   * algorithm */
  struct devol_params params;

};

/* High level entrances to the API. */
int  gene_pool_create(struct gene_pool *pool, int solutions, int threads, 
		      struct devol_params params);
void gene_pool_set_params(struct gene_pool *pool, struct devol_params params);
int  gene_pool_iterate(struct gene_pool *pool);

/* Utility functions for dealing with gene pools. */
double gene_pool_avg_fitness(struct gene_pool *pool);
void   gene_pool_display_fitnesses(struct gene_pool *pool);

/* Functions to be used by the parallel sections of the code. */
void   _gene_pool_calculate_fitnesses_p(struct gene_pool *pool, 
					int start, int stop);

#endif
