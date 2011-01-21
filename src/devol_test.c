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
	      struct solution *dest, struct devol_controller *cont);
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
  .reproduction_rate = .6,
  .breed_fitness = .6,
  .rstate = { 2837, 345, 99 },

};

/* Some made up numbers. */
unsigned short erand48_state[3] = {2674, 14907, 5555};

/* Maximum amount to vary each solution. */
double variance = .005;

int main(int argc, char **argv){

  int i;
  int iter = 0;
  int max_iter = 10000;
  int solutions = 100;

  time_t t_start;
  time_t t_stop;
  struct timeb tmp_time;

  printf("variance=%lf solutions=%d rr=%lf bf=%lf\n",
	 variance, solutions, params.reproduction_rate, params.breed_fitness);

  ftime(&tmp_time);
  t_start = (tmp_time.time * 1000) + tmp_time.millitm;

  /* Make ourselves a gene pool. */
  struct gene_pool pool;
  gene_pool_create(&pool, solutions, 1, params);

  /* And run some iterations... */
  while ( iter++ < max_iter ){
    gene_pool_iterate(&pool);
    printf("%d\t%lf\n", iter, gene_pool_avg_fitness(&pool));
    if ( gene_pool_avg_fitness(&pool) < variance ){
      printf("Convergence in %d generations.\n", iter);
      break;
    }
  }
  if ( iter == max_iter )
    printf("No convergence in %d generations.\n", max_iter);

  ftime(&tmp_time);
  t_stop = (tmp_time.time * 1000) + tmp_time.millitm;

  printf("run time: %ld ms\n", t_stop - t_start);

  /* Print the solutions. */
  _gene_pool_calculate_fitnesses_p(&pool, 0, solutions);
  i = 0;
  /*
  for ( ; i < solutions; i++){
    printf("solution %d: %lf ", i, pool.solutions[i].private.dp_fp);
    printf("fitness: %lf\n", pool.solutions[i].fitness_val);
  }
  */
  return 0;

}

/*
 * This function *must* be reentrant. This function must initialize the 
 * destination solution on its own. This allows optimizations to take place if
 * necessary. Use the controller's random number state and data. Its 
 * deliberatly passed so that each thread can use a renentrant the renentrant
 * erand4_r() function without storing state for each solution.
 */
int mutate(struct solution *par1, struct solution *par2, 
	   struct solution *dest, struct devol_controller *cont){

  double tmp;
  double base;
  double variation;

  /* Pick the better solution of the two and then vary it by a little bit. */
  if ( par1->fitness_val >= par2->fitness_val )
    base = par1->private.dp_fp;
  else 
    base = par2->private.dp_fp;

  /* And vary it by a little bit. */
  erand48_r(cont->rstate, &(cont->rdata), &tmp);
  variation = (tmp * variance) - (variance/2);

  /* Initialize and set the destination solution. */
  dest->private.dp_fp = base + variation;

  return 0;

}

double fitness(struct solution *solution){

  double solution_val = solution->private.dp_fp;

  /* The farther the square is from 5, the worse the solution. */
  return fabs((solution_val * solution_val) - 5);

}

int init(struct solution *solution){

  solution->private.dp_fp = erand48(erand48_state) * 10.0;

  return 0;

}

int destroy(struct solution *solution){

  return 0;

}
