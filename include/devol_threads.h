/*
 * Defines and function prototypes for the thread pool used to spit the work
 * between processors. Fun stuff.
 */

#ifndef _DEVOL_THREADS_H
#define _DEVOL_THREADS_H

#include <pthread.h>

struct thread_pool;

struct devol_controller {

  /* Thread ID. */
  int tid;

  /* Block deliniators into the solution array. */
  int start;
  int stop;

  /* The thread's state. */
  int state;

  /* Set this flag to have the thread terminate conpletely (pthread_exit). */
  int die;

  /* State information for the erand48 function. */
  unsigned short rstate[3];

  /* A pointer back to the thread_pool struct so we can lock against the
   * sync_lock. */
  struct thread_pool *pool;

  /* And also a pointer back to the gene pool for obvious reasons. */
  struct gene_pool *gene_pool;

};

struct thread_pool {

  /* The threads. */
  pthread_t *threads;
  int        thread_count;

  /* A controller for each thread. */
  struct devol_controller *controllers;

  /* A lock to synchronize the thread's work. */
  pthread_mutex_t sync_lock;
  
  /* A flag to mark whether the thread_pool has finished processing the release
   * of the threads and is ready for threads to start terminating. */
  int term_ready;

};

#define DEVOL_TSTATE_WORKING  0   /* In progress. */
#define DEVOL_TSTATE_FINISHED 1   /* The thread is done its iteration. */

/* Thread related functions. */
int thread_pool_init(struct thread_pool *pool, 
		     struct gene_pool *gene_pool, int threads, int solutions);
int thread_pool_destroy(struct thread_pool *pool);

#endif
