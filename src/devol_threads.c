/*
 * Deal with the threading related problems here.
 */

#include <devol.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* Some function prototypes. */
void *_devol_thread_main(void *data);


/*
 * Initialize the the thread pool. Nothing particularly interesting here.
 */
int thread_pool_init(struct thread_pool *pool, int threads){

  int i;
  int err;

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

  /* Finally, start them threads up. */
  for ( i = 0; i < threads; i++){
    pool->controllers[i].tid = i;
    pool->controllers[i].die = 0;
    pool->controllers[i].state = DEVOL_TSTATE_FINISHED;
    pool->controllers[i].pool = pool;
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

  struct devol_controller *controller = (struct devol_controller *)data;

  INFO("Thread (ID=%d) starting up.\n", controller->tid);

  /* This lock forces the thread to wait until the calling algorithm is ready
   * for the thread to start up. */
  pthread_mutex_lock(&(controller->pool->sync_lock));
  pthread_mutex_unlock(&(controller->pool->sync_lock));

  /* This label is used to effectively restart a thread's calculation process.
   */
 run_iteration:
  
  INFO("Thread (ID=%d) starting work block.\n", controller->tid);
  controller->state = DEVOL_TSTATE_WORKING;

  usleep(50000); /* Sleep for half a second, just for testing... */

  /* Make sure the thread_pool is ready to start the thread return... */
  while ( ! controller->pool->term_ready );

  INFO("Thread (ID=%d) entering block termination sequence.\n", 
       controller->tid);

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
