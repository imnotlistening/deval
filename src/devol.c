/*
 * Author: Alex Waterman - imNotListening@gmail.com
 *
 * Handle the general frame work of the algorithm.
 */

#include <devol.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/timeb.h>

unsigned int solution_id = 0;

/*
 * This function is important. It initializes everything. First it initializes
 * the thread pool, this is pretty simple, just a call the the thread_pool
 * initialization function is made.
 */
int  gene_pool_create(struct gene_pool *pool, int solutions, int threads, 
		      struct devol_params params){

  int i, j;
  int err;
  time_t t_start;
  time_t t_stop;
  struct timeb tmp_time;

  /* I lied in the above comment, actually copy in our params first. */
  pool->params = params;

  /* Init the thread pool. */
  err = thread_pool_init(&(pool->workers), pool, threads, solutions);
  if ( err )
    return DEVOL_ERR;

  /* Here is the first use of the call back functions. We allocate a bunch
   * of solutions which we initialize with the provided call back function. */
  pool->solutions = (solution_t *)malloc(sizeof(solution_t) * solutions);
  if ( ! pool->solutions )
    return DEVOL_ERR;

  pool->solution_count = solutions;

  ftime(&tmp_time);
  t_start = (tmp_time.time * 1000) + tmp_time.millitm;
  INFO("Generating %d initial solutions... ", solutions);
  for ( i = 0; i < solutions; i++){

    pool->solutions[i].mutate = params.mutate;
    pool->solutions[i].fitness = params.fitness;
    pool->solutions[i].init = params.init;
    pool->solutions[i].destroy = params.destroy;

    /* Figure out which controller this solution belongs to. */
    for ( j = 0; j < threads; j++){
      if ( i >= pool->workers.controllers[j].start &&
	   i <  pool->workers.controllers[j].stop )
	pool->solutions[i].cont = &(pool->workers.controllers[j]);
    }

    params.init(&(pool->solutions[i]));

  }
  INFO("Done\n");
  ftime(&tmp_time);
  t_stop = (tmp_time.time * 1000) + tmp_time.millitm;
  INFO("Time to allocate initial solutions: %ld ms\n", t_stop - t_start);

  pool->flags = GPOOL_SMP;

  return DEVOL_OK;

}

/*
 * Like gene_pool_create(), this function will initialize a gene_pool for use.
 * however, it will make it specifically for use with the single threaded
 * version. Do not use a seq gene_pool with the SMP iterate function, and do
 * not use a smp gene_pool with the seq version. Doing so will result in an
 * error being returned and no computation being done.
 */
int  gene_pool_create_seq(struct gene_pool *pool, int solutions,
			  struct devol_params params){

  int i;
  time_t t_start;
  time_t t_stop;
  struct timeb tmp_time;

  pool->params = params;

  /* Instead of init'ing the thread pool, just memset it to 0. */
  memset(&(pool->workers), 0, sizeof(struct thread_pool));

  /* Here is the first use of the call back functions. We allocate a bunch
   * of solutions which we initialize with the provided call back function. */
  pool->solutions = (solution_t *)malloc(sizeof(solution_t) * solutions);
  if ( ! pool->solutions )
    return DEVOL_ERR;

  pool->solution_count = solutions;

  /* Here we calculate the gene_pool characteristics: breeder_window,
   * reproduction rate, etc, and allocate out the new_colutions memory. We do
   * this before we initialize the initial population of solutions so that if
   * this call fails, we wont have to deinit the population before freeing
   * that memory. */
  pool->new_count = (int)(params.reproduction_rate * solutions);
  pool->breeder_window = (int)(params.breed_fitness * solutions);
  pool->new_solutions = 
    (solution_t *)malloc(sizeof(solution_t) * pool->new_count);
  if ( ! pool->new_solutions ){
    free(pool->solutions);
    return DEVOL_ERR;
  }

  /* Now we must initialize the gene_pool controller. */
  pool->controller.tid = 0;
  pool->controller.start = 0;
  pool->controller.stop = pool->solution_count;
  pool->controller.pool = NULL; /* NULL thread pool. */
  pool->controller.gene_pool = pool;
  pool->controller.rstate[0] = params.rstate[0];
  pool->controller.rstate[1] = params.rstate[1];
  pool->controller.rstate[2] = params.rstate[2];
  memset(&(pool->controller.rdata), 0, sizeof(struct drand48_data)); 

  ftime(&tmp_time);
  t_start = (tmp_time.time * 1000) + tmp_time.millitm;
  INFO("Generating %d initial solutions... ", solutions);
  for ( i = 0; i < solutions; i++){

    pool->solutions[i].mutate = params.mutate;
    pool->solutions[i].fitness = params.fitness;
    pool->solutions[i].init = params.init;
    pool->solutions[i].destroy = params.destroy;
    pool->solutions[i].cont = &pool->controller;

    params.init(&(pool->solutions[i]));

  }
  INFO("Done\n");
  ftime(&tmp_time);
  t_stop = (tmp_time.time * 1000) + tmp_time.millitm;
  INFO("Time to allocate initial solutions: %ld ms\n", t_stop - t_start);

  pool->flags = GPOOL_SEQ;

  return DEVOL_OK;


}

/*
 * Here is the sequential version of the evolutionary algorithm. The multi
 * threaded version is in devol_threads.c.
 *
 * Some assumptions this function makes. When created in sequential mode, a
 * gene_pool will have one controller and it will have the requried sequential
 * data precalculated (breeder_window, etc).
 */
int gene_pool_iterate_seq(struct gene_pool *pool){

  int i;
  int s1_ind, s2_ind, die_ind;
  double tmp;
  solution_t *s1, *s2, *die;

  /*
   * Here is where we start doing the work. The algorithm is as follows:
   *
   *  1) Calculate each solutions fitness.
   *  2) Sort our block by fitness. The closer to 0 the fitness value, the
   *       better the solution.
   *  3) Create new solutions by breeding good solutions randomly.
   *  4) Replace the worst solutions with the newly created solutions.
   */
  _gene_pool_calculate_fitnesses_p(pool, 0, pool->solution_count);

  /* Sort the solutions. */
  qsort(pool->solutions, pool->solution_count, 
	sizeof(solution_t), _compare_solutions);

  /* Make some new solutions. */
  for ( i = 0; i < pool->new_count; i++){

    /* Generate the two parent solution indexes. */
    tmp = erand48(pool->controller.rstate);
    s1_ind = (int)(tmp * pool->breeder_window);
    do {
      tmp = erand48(pool->controller.rstate);
      s2_ind = (int)(tmp * pool->breeder_window);
    } while (s1_ind == s2_ind);

    /* Get the parent solution addresses. */
    s1 = (solution_t *)&(pool->solutions[s1_ind]);
    s2 = (solution_t *)&(pool->solutions[s2_ind]);

    /* And make a new solution. */
    s1->mutate(s1, s2, &(pool->new_solutions[i]));
    pool->new_solutions[i].mutate = s1->mutate;
    pool->new_solutions[i].fitness = s1->fitness;
    pool->new_solutions[i].init = s1->init;
    pool->new_solutions[i].destroy = s1->destroy;
    pool->new_solutions[i].cont = &pool->controller;

    /* Now that we have a solution, find another solution to kill and 
       replace. */
    tmp = erand48(pool->controller.rstate);
    die_ind = 1 + ((int)(tmp * pool->new_count));
    die_ind = pool->solution_count - die_ind;
    die = (solution_t *)&(pool->solutions[die_ind]);
    die->destroy(die);

    *die = pool->new_solutions[i];

  }

  return DEVOL_OK;

}

/*
 * Randomly disperse some of the solutions around. This forces different
 * parts of the population to breed together. This is where we try and make up
 * for the fact that the algrithm, when run in parallel, really is only using
 * much much smaller populations, which hurts convergence.
 */
void gene_pool_disperse(struct gene_pool *pool){

  int disperse;

  disperse = (int)(pool->params.gene_dispersal_factor * pool->solution_count);

  while ( disperse > 0 ){

    /* Pick two solutions and swap them. */
    

  }

}
