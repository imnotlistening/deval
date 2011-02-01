/*
 * Defines and functions for the mixture solving algorithm.
 */

#ifndef _MIXTURE_H
#define _MIXTURE_H

#include <stdint.h>
#include <stddef.h>

/*
 * A struct for describing each normal distribution expected in the mixture.
 */
struct normal {

  double mu_min;
  double mu_max;

  double sigma_min;
  double sigma_max;

  double mu_var;
  double sigma_var;

  char name[64];

};

/*
 * A struct to hold private data for each solution.
 */
struct mixture_solution {

  double *mu;    /* List of mus for each distribution. */
  double *sigma; /* List of stddevs for each distribution. */
  double *prob;  /* Probability for the given distribution. */
  int len;       /* How many distributions we have. */

  /* Store this once we have calculated the result so we dont have to do
   * it again. */
  int solved;
  double mle;

};

/*
 * Padded random state for the init() function. Pad to 64 bytes.
 */
struct padded_rstate {

  /* 48 bits = 6 bytes. */
  unsigned short rstate[3];

  /* 64 - 6 = 58 bytes. */
  char __padding[58];

};

/* Statically define the probability variance. Add this to the arguement list
 * later on. */
#define PROB_VAR (.01)

/* This is the maximum fitness ceiling. Fitness is defined as how close a
 * solution is to this value. If fitnesses values go over this, then the
 * algorithm will not work.
 */
#define FITNESS_CEILING (1.0e12)

/*
 * A special memory allocator. Lockless and threadable but highly specialized.
 */
struct bucket {

  void     *base_addr;
  uint32_t *alloc_table;
  size_t    elems;

};

struct bucket_table {

  int bucket_count;
  size_t block_size;

  struct bucket *buckets;
  
  /* The real base of memory. We will allocate all bucket's memory once. */
  void *base;

  /* All the allocation tables. */
  uint32_t *alloc_tables;

};

/*
 * Functions to use.
 */
struct normal *read_mixture_file(char *file, int *norms);
double        *read_data_file(char *file, int *samples);
int            init_bucket_allocator(struct bucket_table *tbl, int buckets,
				     size_t block_size, size_t elems);

#endif
