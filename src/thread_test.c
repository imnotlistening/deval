/*
 * Test the functionality of the thread pool. This is pretty crucial. The
 * thread pool has to be 100%.
 */

#include <devol.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(){

  int i;
  int err;
  int done;
  int waiting;

  int threads = 4;
  int iterations = 10;

  struct thread_pool pool;

  err = thread_pool_init(&pool, threads); /* n threads. */
  if ( err ){
    printf("Unable to init the thread pool. :(\n");
    return 1;
  }

  /* So... did it work... */
  //usleep(999999);

  /* Run a few iterations of the thread algorithm to see if it works. */
  while ( iterations-- > 0 ){

    printf("Iteration: %d\n", iterations);

    /* Set term_ready to false. */
    pool.term_ready = 0;

    /* Release the sync_lock lock. */
    pthread_mutex_unlock(&(pool.sync_lock));

    /* Now we wait until each thread has started. */
    waiting = 1;
    while ( waiting ){

      done = 0;
      
      for ( i = 0; i < threads; i++){
	if ( pool.controllers[i].state != DEVOL_TSTATE_WORKING ){
	  done = 1;
	  break;
	}
      }

      waiting = done;

    }

    /* Here each thread has started, so now we should be able to relock the
     * sync_lock and tell the threads that they can finish. */
    pthread_mutex_lock(&(pool.sync_lock));
    pool.term_ready = 1;

    /* Now we wait until each thread has terminated. */
    waiting = 1;
    while ( waiting ){

      done = 0;
      
      for ( i = 0; i < threads; i++){
	if ( pool.controllers[i].state != DEVOL_TSTATE_FINISHED ){
	  done = 1;
	  break;
	}
      }

      waiting = done;

    }

  }

  /* Now kill all the threads in the thread pool. */
  thread_pool_destroy(&pool);

  return 0;

}
