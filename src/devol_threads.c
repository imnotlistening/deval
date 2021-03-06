/*
 * Deal with the threading related problems here.
 */

#include <devol.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/timeb.h>

/* Some function prototypes. */
void *_devol_thread_main(void *data);

/*
 * Initialize the the thread pool. Nothing particularly interesting here.
 */
int thread_pool_init(struct thread_pool *pool, 
		     struct gene_pool *gene_pool, int threads, int solutions){

  int i;
  int err;
  int block_size;
  int start, stop;

  pool->thread_count = threads;
  pool->term_ready = 0;

  /* First thing we have to do is make the lock. We also lock it so when the
   * worker threads are fist created, they do not start doing undefined stuff.
   */
  pthread_mutex_init(&(pool->sync_lock), NULL);
  pthread_mutex_lock(&(pool->sync_lock));

  /* Now allocate out some thread data... */
  pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * threads);
  if ( ! pool->threads )
    return DEVOL_ERR;

  /* And their controllers. */
  pool->controllers = (struct devol_controller *)
    malloc(sizeof(struct devol_controller) * threads);
  if ( ! pool->controllers ){
    free(pool->threads); /* Clean up... Boring. */
    return DEVOL_ERR;
  }

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
    pool->controllers[i].start = start;
    pool->controllers[i].stop = stop;
    start = stop;
    stop += block_size;
  }

  /* 
   * Give any extras solutions to the final thread. Unless there are a 
   * staggering number of threads this really shouldnt be a problem. 
   */
  pool->controllers[threads-1].stop = solutions;  

  /* Finally, start them threads up. */
  for ( i = 0; i < threads; i++){
    pool->controllers[i].tid = i;
    pool->controllers[i].die = 0;
    pool->controllers[i].state = DEVOL_TSTATE_FINISHED;
    pool->controllers[i].pool = pool;
    pool->controllers[i].gene_pool = gene_pool;
    pool->controllers[i].rstate[0] = gene_pool->params.rstate[0] + i;
    pool->controllers[i].rstate[1] = gene_pool->params.rstate[1] + i+1;
    pool->controllers[i].rstate[2] = gene_pool->params.rstate[2] + i+2;
    memset(&(pool->controllers[i].rdata), 0, sizeof(rdata_t));
    err = pthread_create( &(pool->threads[i]), NULL, _devol_thread_main, 
			  &(pool->controllers[i]));
  }

  return DEVOL_OK;

}

/*
 * Cleanly destroy a thread pool. You *MUST* have the lock to the thread pool
 * (sync_lock) in order to call this function. Otherwise, bad things may
 * happen.
 */
int thread_pool_destroy(struct thread_pool *pool){

  int i;

  /* Tell each controller to kill their threads. */
  for ( i = 0; i < pool->thread_count; i++){
    pool->controllers[i].die = 1;
  }

  /* And release the sync_lock. */
  pthread_mutex_unlock(&(pool->sync_lock));

  /* Wait for each thread to die... */
  for ( i = 0; i < pool->thread_count; i++){
    pthread_join(pool->threads[i], NULL);
  }

  /* Now free the thread pool memory. */
  free(pool->threads);
  free(pool->controllers);

  return DEVOL_OK;

}

/*
 * This is the function that does all of the work related to the evolutionary
 * algorithm. This function must be reentrant (DUH) since it will be called
 * several times in parallel. This function needs to implement all of the logic
 * associated with keeping a thread pool going. The goal is to not keep making
 * new threads every iteration; instead it is more ideal to give the threads
 * their tasks and simply let them at the problem, iteration by iteration.
 */
void *_devol_thread_main(void *data){

  double tmp;
  double rrate;
  double bfitness;
  int breeder_window;
  solution_t *new_solutions;
  int solution_count, i;
  solution_t *s1, *s2, *die;
  int s1_ind, s2_ind;
  int die_index;

#ifdef _TIMING
  time_t t_start;
  time_t t_delta;
  struct timeb tmp_time;
#endif

  struct devol_controller *controller = (struct devol_controller *)data;

  INFO("Thread (ID=%d) starting up.\n", controller->tid);
  INFO(" (ID=%d) Block allocation: %d -> %d\n", controller->tid,
       controller->start, controller->stop);

  /* Set up our params. */
  rrate = controller->gene_pool->params.reproduction_rate;
  bfitness = controller->gene_pool->params.breed_fitness;

  breeder_window = (int)(bfitness * (controller->stop - controller->start));
  solution_count = (int)(rrate * (controller->stop - controller->start));

  /* Allocate out space for new solutions. */
  new_solutions = (solution_t *)malloc(sizeof(solution_t) * solution_count);
  INFO("(ID=%d) Solution count: %d\n", controller->tid, solution_count);
  INFO("(ID=%d) Breeding window: %d\n", controller->tid, breeder_window);

  /* This lock forces the thread to wait until the calling algorithm is ready
   * for the thread to start up. */
  pthread_mutex_lock(&(controller->pool->sync_lock));
  pthread_mutex_unlock(&(controller->pool->sync_lock));

  /* This label is used to effectively restart a thread's calculation process.
   */
 run_iteration:

#ifdef _TIMING
  ftime(&tmp_time);
  t_start = (tmp_time.time * 1000) + tmp_time.millitm;
#endif

  controller->state = DEVOL_TSTATE_WORKING;

  /*
   * Here is where we start doing the work. The algorithm is as follows:
   *
   *  1) Calculate each solutions fitness.
   *  2) Sort our block by fitness. The closer to 0 the fitness value, the
   *       better the solution.
   *  3) Create new solutions by breeding good solutions randomly.
   *  4) Replace the worst solutions with the newly created solutions.
   */
  _gene_pool_calculate_fitnesses_p(controller->gene_pool, 
				   controller->start, controller->stop);
#ifdef _TIMING
  ftime(&tmp_time);
  t_delta = (tmp_time.time * 1000) + tmp_time.millitm;
  INFO("(ID=%d) Computed fitnesses (delta_t=%d).\n", 
       controller->tid, (int)(t_delta - t_start));
#endif

  /*
   * This could potentially give rise to superlinear speedups. This is because
   * sort algorithms scale super-linearly with problem size. I.e the run time
   * for qsort is O(N * log(N)). Thus as N gets larger, the sequential program
   * starts to hurt more than k subsections of an N sized problem.
   */
  qsort(&(controller->gene_pool->solutions[controller->start]), 
	controller->stop - controller->start,
	sizeof(solution_t), _compare_solutions);
#ifdef _TIMING
  ftime(&tmp_time);
  t_delta = (tmp_time.time * 1000) + tmp_time.millitm;
  INFO("(ID=%d) Sorted (delta_t=%d).\n", controller->tid, 
       (int) (t_delta - t_start));
#endif

  /* This is kinda complex... basically we have to randomly choose some of the
   * the better solutions to breed. This is affected by the param 
   * reproduction_rate. The higher the reproduction rate, the more solutions
   * we make per generation. 
   */
  INFO("%d new solutions...\n", solution_count);
  for ( i = 0; i < solution_count; i++){

    /* Generate a new solution from the two randomly selected in the
     * breeder_window. */
    devol_rand48(controller->rstate, &(controller->rdata), &tmp);
    s1_ind = (int)(tmp * breeder_window);
    do {
      devol_rand48(controller->rstate, &(controller->rdata), &tmp);
      s2_ind = (int)(tmp * breeder_window);
    } while (s1_ind == s2_ind);

    /* Get the addresses of the solution data in the solution pool of the
     * gene pool. The sort will put the better solutions in the lower indexes
     * of our block, thus we need only use the start of our block and add the
     * random component of our index in order to get the random solution in the
     * breeder window. */
    s1 = (solution_t *) 
      &(controller->gene_pool->solutions[controller->start + s1_ind]);
    s2 = (solution_t *) 
      &(controller->gene_pool->solutions[controller->start + s2_ind]);
    DEBUG("Mutating solutions: %d(%lf) and %d(%lf).\n", 
	  s1_ind, s1->fitness_val, 
	  s2_ind, s2->fitness_val);

    /* And make the new solution. */
    new_solutions[i].mutate = s1->mutate;
    new_solutions[i].fitness = s1->fitness;
    new_solutions[i].init = s1->init;
    new_solutions[i].destroy = s1->destroy;
    new_solutions[i].cont = controller;
    s1->mutate(s1, s2, &(new_solutions[i]));

    /* Now choose a solution to die and be replaced. We will start killing
     * solutions starting with the bad. We will wrap around if necessary; i.e:
     * more solutions are bred than we have room for. */
    die_index = controller->stop - (i % breeder_window) - 1;

    DEBUG("  Killing %d\n", die_index);
    die = (solution_t *) &(controller->gene_pool->solutions[die_index]);
    die->destroy(die);

    /* And finally do the replacement. */
    *die = new_solutions[i];

  }
#ifdef _TIMING
  ftime(&tmp_time);
  t_delta = (tmp_time.time * 1000) + tmp_time.millitm;
  INFO("(ID=%d) Breeded (delta_t=%d).\n", controller->tid, 
       (int) (t_delta - t_start));
#endif

  _gene_pool_calculate_fitnesses_p(controller->gene_pool, 
				   controller->start, controller->stop);
#ifdef _TIMING
  ftime(&tmp_time);
  t_delta = (tmp_time.time * 1000) + tmp_time.millitm;
  INFO("(ID=%d) Done generation (delta_t=%d).\n", 
       controller->tid, (int) (t_delta - t_start));
#endif

  /* Make sure the thread_pool is ready to start the thread return... */
  while ( ! controller->pool->term_ready );

  /* Annouce that we are done, and wait until we are released to start again.
   * When we get the sync_lock we assume we are ready to go again. */
  controller->state = DEVOL_TSTATE_FINISHED;
  pthread_mutex_lock(&(controller->pool->sync_lock));
  pthread_mutex_unlock(&(controller->pool->sync_lock));

  /* Good bye cruel world. */
  if ( controller->die ){
    INFO("Killing thread: tid=%d\n", controller->tid);
    pthread_exit(0);
  }

  goto run_iteration;

  /* Unreachable, but to make the compiler happy... */
  return NULL;

}

/*
 * Here we do an iteration of the algorithm. Call this function over and over
 * to solve an evolutionary problem.
 */
int gene_pool_iterate(struct gene_pool *gene_pool){

  int i;
  int done;
  int waiting;

  /* Make sure threads don't finish before we are ready for them to finish
   * i.e reaquired the sync_lock. */
  gene_pool->workers.term_ready = 0;
  
  /* Release the HOUNDS!!! */
  pthread_mutex_unlock(&(gene_pool->workers.sync_lock));

  /* Wait until all of the threads have started. */
  waiting = 1;
  while ( waiting ){
     
    done = 0;
    usleep(5); /* Wait half a millisecond... */
      
    for ( i = 0; i < gene_pool->workers.thread_count; i++){
      if ( gene_pool->workers.controllers[i].state != DEVOL_TSTATE_WORKING ){
	done = 1;
	break;
      }
    }

    waiting = done;

  } 

  /* Here each thread has started, so now we should be able to relock the
   * sync_lock and tell the threads that they can finish. */
  pthread_mutex_lock(&(gene_pool->workers.sync_lock));
  gene_pool->workers.term_ready = 1;

  /* Now we wait until each thread has terminated. */
  waiting = 1;
  while ( waiting ){

    done = 0;
    usleep(5); /* Check if the threads are done sometimes. */
      
    for ( i = 0; i < gene_pool->workers.thread_count; i++){
      if ( gene_pool->workers.controllers[i].state != DEVOL_TSTATE_FINISHED ){
	done = 1;
	break;
      }
    }

    waiting = done;

  }

  /* Finally, we should do some gene dispersal. Each population of solutions
   * are isolated duing the normal operation of the algorithm. This is like
   * birds on islands. Here we try and get some birds to travel to other 
   * islands, so to speak. */
  gene_pool_disperse(gene_pool);

  return DEVOL_OK;

}
