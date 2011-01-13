/*
 * Author: Alex Waterman - imNotListening@gmail.com
 *
 * Handle the general frame work of the algorithm.
 */

#include <devol.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/*
 * This function is important. It initializes everything. First it initializes
 * the thread pool, this is pretty simple, just a call the the thread_pool
 * initialization function is made.
 */
int  gene_pool_create(struct gene_pool *pool, int solutions, int threads, 
		      struct devol_params params){

  int i;
  int err;
  int block_size;
  int start, stop;

  /* I lied in the above comment, actually copy in our params first. */
  pool->params = params;

  /* Init the thread pool. */
  err = thread_pool_init(&(pool->workers), pool, threads);
  if ( err )
    return DEVOL_ERR;

  /* Here is the first use of the call back functions. We allocate a bunch
   * of solutions which we initialize with the provided call back function. */
  pool->solutions = (solution_t *)malloc(sizeof(solution_t) * solutions);
  if ( ! pool->solutions )
    return DEVOL_ERR;

  pool->solution_count = solutions;

  INFO("Generating %d initial solutions... ", solutions);
  for ( i = 0; i < solutions; i++){

    pool->solutions[i].mutate = params.mutate;
    pool->solutions[i].fitness = params.fitness;
    pool->solutions[i].init = params.init;
    pool->solutions[i].destroy = params.destroy;

    params.init(&(pool->solutions[i]));

  }
  INFO("Done\n");

  /* 
   * We have to allocate out blocks of the gene pool to each thread. Thus we
   * must figure out exactly where each block starts and ends. This is more of
   * a pain that I thought it would be. Oh, then make sure the thread 
   * controllers are updated with these values.
   */
  block_size = solutions / threads;
  start = 0;
  stop = block_size;

  for ( i = 0; i < threads; i++){
    pool->workers.controllers[i].start = start;
    pool->workers.controllers[i].stop = stop;
    start = stop;
    stop += block_size;
  }

  /* 
   * Give any extras solutions to the final thread. Unless there are a 
   * staggering number of threads this really shouldnt be a problem. 
   */
  pool->workers.controllers[threads-1].stop = solutions;  

  printf("Block allocations:\n");
  for ( i = 0; i < threads; i++){
    printf(" block %d: %d -> %d\n", i, pool->workers.controllers[i].start,
	   pool->workers.controllers[i].stop);
  }

  return DEVOL_OK;

}

