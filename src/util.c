/*
 * Some utility functions for dealing with gene pools.
 */

#include <devol.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

double gene_pool_avg_fitness(struct gene_pool *pool){

  int i;
  double total = 0.0;

  if ( ! pool )
    return 0;

  for ( i = 0; i < pool->solution_count; i++){
    pool->solutions[i].fitness_val = 
      pool->solutions[i].fitness(&(pool->solutions[i]));
    total += pool->solutions[i].fitness_val;
  }

  return total / pool->solution_count;

}

void gene_pool_display_fitnesses(struct gene_pool *pool){

  int i;

  if ( ! pool )
    return;

  for ( i = 0; i < pool->solution_count; i++){
    pool->solutions[i].fitness_val = 
      pool->solutions[i].fitness(&(pool->solutions[i]));
    printf("Solution %5d: fitness=%lf\n", i, pool->solutions[i].fitness_val);
  }

}

void _gene_pool_calculate_fitnesses_p(struct gene_pool *pool, 
				      int start, int stop){

  int i;

  if ( ! pool )
    return;

  for ( i = start; i < stop; i++)
    pool->solutions[i].fitness_val = 
      pool->solutions[i].fitness(&(pool->solutions[i]));

}

int _compare_solutions(const void *a, const void *b){

  solution_t *s_a = (solution_t *)a;
  solution_t *s_b = (solution_t *)b;

  if ( s_a->fitness_val < s_b->fitness_val )
    return -1;
  if ( s_a->fitness_val > s_b->fitness_val )
    return 1;
  return 0;

}

void devol_rand48(unsigned short rstate[3], rdata_t *rdata, double *d){

#ifdef __linux__
  erand48_r(rstate, rdata, d);
#else
  *d = erand48(rstate);
#endif

}
