/*
 * Test out the general framework of the algorithm. Nothing mind shattering
 * here. See if we can solve the square root of 5.
 */

#include <devol.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/timeb.h>

/* Prototypes for the call backs. */
int    mutate(struct solution *par1, struct solution *par2, 
	      struct solution *dest);
double fitness(struct solution *solution);
int    init(struct solution *solution);
int    destroy(struct solution *solution);

/* Some data. */
struct devol_params params = {

  .mutate = mutate,
  .fitness = fitness,
  .init = init,
  .destroy = destroy,

  .gene_dispersal_factor = 0.0,
  .reproduction_rate = .30,
  .breed_fitness = .30,

};

/* Some made up numbers. */
unsigned short erand48_state[3] = {2674, 1507, 5555};

/* Maximum amount to vary each solution. */
double variance = .0005;

int main(){

  int i;
  int iter = 0;
  int max_iter = 200;

  time_t t_start;
  time_t t_stop;
  struct timeb tmp_time;

  ftime(&tmp_time);
  t_start = (tmp_time.time * 1000) + tmp_time.millitm;

  /* Make ourselves a gene pool. */
  struct gene_pool pool;
  gene_pool_create(&pool, 800000, 2, params);

  /* And run some iterations... */
  while ( iter++ < max_iter ){
    gene_pool_iterate(&pool);  
    printf("%d\t%lf\n", 
	   iter, gene_pool_avg_fitness(&pool));
  }

  ftime(&tmp_time);
  t_stop = (tmp_time.time * 1000) + tmp_time.millitm;

  printf("run time: %ld ms\n", t_stop - t_start);

  /* Print the solutions. */
  i = 0;
  /*
  for ( ; i < 100; i++){
    printf("solution %d: %lf ", i, *((double *)pool.solutions[i].private));
    printf("fitness: %lf\n", pool.solutions[i].fitness_val);
  }
  */
  return 0;

}

/*
 * This function *must* be reentrant. This function must initialize the 
 * destination solution on its own. This allows optimizations to take place if
 * necessary.
 */
int mutate(struct solution *par1, struct solution *par2, 
	   struct solution *dest){

  double base;
  double variation;
  double *solution_val;

  /* Pick the better solution of the two and then vary it by a little bit. */
  base = par1->fitness_val;
  if ( par2->fitness_val < base )
    base = par2->fitness_val;

  /* And vary it by a little bit. */
  variation = (erand48(erand48_state) * variance) - (variance/2);

  /* Initialize and set the destination solution. */
  dest->private = malloc(sizeof(double));
  solution_val = (double *)dest->private;
  *solution_val = *((double *)par1->private) + variation;

  return 0;

}

double fitness(struct solution *solution){

  double solution_val = (double)*((double *)solution->private);

  /* The farther the square is from 5, the worse the solution. */
  return fabs((solution_val * solution_val) - 5);

}

int init(struct solution *solution){

  double *data;

  /* Allocate memory for a double precision floating point value. */
  solution->private = malloc(sizeof(double));
  data = (double *)solution->private;

  *data = erand48(erand48_state) * 10.0;

  return 0;

}

int destroy(struct solution *solution){

  if ( solution->private )
    free(solution->private);
  else
    printf("Ignoring empty solution...\n");

  return 0;

}
