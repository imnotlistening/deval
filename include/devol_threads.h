/*
 * Defines and function prototypes for the thread pool used to spit the work
 * between processors. Fun stuff.
 */

#ifndef _DEVOL_THREADS_H
#define _DEVOL_THREADS_H

#include <stdlib.h>
#include <pthread.h>

struct thread_pool;

/*
 * Since this struct will be getting a *lot* of concurrent access (possibly),
 * it must be aligned and/or padded out to a multiple of cacheline sizes. For
 * now I will assume the largest cache lines in existence are 256 bytes and
 * that smaller cache lines will be factors of 256 (128, 64, etc).
 */
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

  /* State information for the erand48_r function. */
  unsigned short rstate[3];
  struct drand48_data rdata;

  /* A pointer back to the thread_pool struct so we can lock against the
   * sync_lock. */
  struct thread_pool *pool;

  /* And also a pointer back to the gene pool for obvious reasons. */
  struct gene_pool *gene_pool;


  /* Pad this struct out so that it is exactly 128 bytes. */
#ifdef __x86_64__
  char __padding[52]; /* I can't imagine cache lines > 128 bytes. */
#else
  char __padding[68];
#endif

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
