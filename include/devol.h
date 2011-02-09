
/*
 * Data structure layouts. This stuff is important.
 */

#ifndef _DEVOL_H

#include <stdlib.h>

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

/* Deal with SunOS. Gah. */
#ifdef __sun__
typedef unsigned short rdata_t[7];
#else
typedef struct drand48_data rdata_t;
#endif

/* Now we can include the thread stuff. */
#include <devol_threads.h>

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
  void   (*swap)(struct solution *left, struct solution *right);

  /* The calculated fitness value. */
  double fitness_val;

  /* The solution's private data. Use what ever you want... */
  union {
    long unsigned int  uint_64; /* 64 Bit integer. */
    void              *ptr;     /* A pointer to something else  */
    double             dp_fp;   /* A double precision floating point. */
  } private;

  /* The solution's controller. */
  struct devol_controller *cont;

};

typedef struct solution solution_t;

/*
 * A struct for defining and passing various paramaters for the genetic
 * algorithm.
 */
struct devol_params {

  /* Pass in the call back functions that will be propagated to each individual
   * solution. */
  int    (*mutate)(solution_t *par1, solution_t *par2, 
		   solution_t *dest);
  double (*fitness)(solution_t *solution);
  int    (*init)(solution_t *solution);
  int    (*destroy)(solution_t *solution);
  void   (*swap)(solution_t *left, solution_t *right);

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

  /*
   * This allows the calling program to specify the random state.
   */
  unsigned short rstate[3];

};

/*
 * This struct defines a pool of solutions. The call back function pointers
 * are used to initialize and manipulate a population of solutions.
 */
struct gene_pool {

  /* The soltuions themselves. */
  solution_t *solutions;
  size_t      solution_count;

  /* Some flags. */
  unsigned int flags;
  
  /* A pool of threads to use for distributing the work. */
  struct thread_pool workers;

  /* An aggragation of the myriad paramaters that go into an evolutionary
   * algorithm */
  struct devol_params params;

  /*
   * Data for the *sequential* algorithm only. This data is held in the 
   * thread's entrance function's stack frame for the multithreaded version.
   */
  solution_t *new_solutions;
  int         new_count;
  int         breeder_window;

  /* The gene_pool controller. Only needed and initialized if the gene_pool is
   * going to be sequential. */
  struct devol_controller controller;

};

/* Flag definitions for the gene_pool struct. */
#define GPOOL_SEQ   0
#define GPOOL_SMP   1

/* High level entrances to the API. */
int  gene_pool_create(struct gene_pool *pool, int solutions, int threads, 
		      struct devol_params params);
int  gene_pool_create_seq(struct gene_pool *pool, int solutions,
			  struct devol_params params);
void gene_pool_set_params(struct gene_pool *pool, struct devol_params params);
int  gene_pool_iterate(struct gene_pool *pool);
int  gene_pool_iterate_seq(struct gene_pool *pool);

/* Utility functions for dealing with gene pools. */
double gene_pool_avg_fitness(struct gene_pool *pool);
void   gene_pool_display_fitnesses(struct gene_pool *pool);
void   gene_pool_disperse(struct gene_pool *pool);
int    _compare_solutions(const void *a, const void *b);
void   devol_rand48(unsigned short rstate[3], rdata_t *rdata, double *d);

/* Functions to be used by the parallel sections of the code. */
void   _gene_pool_calculate_fitnesses_p(struct gene_pool *pool, 
					int start, int stop);

#endif
