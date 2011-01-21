/*
 * Header for the grid evolutionary computing constructs.
 */

#include <pthread.h>

#include <netinet/in.h>

#ifndef _DEVOL_GRID_H
#define _DEVOL_GRID_H

/*
 * A struct to keep track of a node that is connected to us.
 */
struct grid_node {

  /* The connected host. */
  struct sockaddr_in host;

  /* The state of the host, busy, available, down, etc. */
  int state;

  /* A file descriptor to use to read/write to/from the host. */
  int fd;

  /* A lock to make sure access to a node is synchronous. */
  pthread_mutex_t node_lock;

};

#define DEVOL_GRID_NOEXT    0x0      /* There is no host. */
#define DEVOL_GRID_AVAIL    0x1      /* The host is available for work. */
#define DEVOL_GRID_BUSY     0x2      /* The host is busy processing. */
#define DEVOL_GRID_DOWN     0x4      /* The host appears to be down. */

/*
 * Behind the scenes grid computing queue. This struct lays out the required
 * information to farm out jobs to a grid of computing nodes.
 */
struct grid_queue {

  /* The name of the command that this queue is executing on its hosts. */
  char *command; 

  /* The nodes that are connected. */
  struct grid_node *nodes;
  int n_count;

  /* A file conatining a list of nodes to connect to. */
  char *node_list;

};


#endif
