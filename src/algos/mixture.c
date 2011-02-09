/*
 * Solve a mixture problem. Given a data set, attempt to find what paramters
 * for some set of normal distributions give us the maximum liklyhood of the
 * data set.
 *
 * Relevant parameters:
 *
 *   data          <file>               The file containing the data set.
 *   norms         <file>               A file describing the number and ranges
 *                                      of the distributions.
 *   pop-size      <integer>            The population size.
 *   rep-rate      <double>             The reproduction rate of the pop.
 *   dispersal     <integer>            The amount of gene dispersal.
 *   breed-fitness <double>             Percent of the population that is
 *                                      allowed to breed.
 *   max-iter      <integer>            Maximum iterations.
 *   seed          <s1,s2,s3>           3 unsigned short integer seed values
 *                                      for the drand48() family of random 
 *                                      functions.
 *   converge      N/A                  If specified terminate the algorithm
 *                                      when the average population fitness is
 *                                      less than variance.
 *   verbose       N/A                  Will be verbose.
 *   help          N/A                  Display a help message.
 *
 * The file containing the data should be a list of number seperated by newline
 * characters. The file containing the distributions should be a list seperated
 * by newline of the following:
 * 
 *   <name> <(mu min,mu max)> <(sigma min, sigma max)> <mu var> <sigma var>
 *
 * For example:
 *
 *   my_dist (-1,1) (-3,3) .001 .005
 */

#include <devol.h>

/* Our own little header file, not part of the evolutionary stuff. */
#include <mixture.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <sys/timeb.h>

/*
 * Prototypes for functions we need.
 */
int     mutate(solution_t *par1, solution_t *par2, 
	       solution_t *dest);
double  fitness(solution_t *solution);
int     init(solution_t *solution);
int     destroy(solution_t *solution);
void    swap(solution_t *left, solution_t *right);
int    *parse_integer_array(char *list, int *count);
void    die(char *msg);
int     run();
void    print_solution(solution_t *s);

/*
 * Fields that modify the functionality of the program.
 */
int converge = 0;
int verbose  = 0;
int defaults = 0;
int help     = 0;

int pop_size = 100;
int max_iter = 100;
int threads  = 1;

char *data_file = NULL;
char *norms_file = NULL;

/*
 * These are the arguements themselves.
 */
struct option mix_opts[] = {
  
  {"data", 1, NULL, 'd'},
  {"norms", 1, NULL, 'n'},
  {"pop-size", 1, NULL, 'p'},
  {"rep-rate", 1, NULL, 'r'},
  {"dispersal", 1, NULL, 'D'},
  {"threads", 1, NULL, 't'},
  {"breed-fitness", 1, NULL, 'b'},
  {"max-iter", 1, NULL, 'm'},
  {"seed", 1, NULL, 's'},
  {"converge", 0, &converge, 'C'},
  {"verbose", 0, &verbose, 'v'},
  {"help", 0, &help, 'h'},
  {NULL, 0, NULL, 0},

};
char *args = "d:n:p:r:t:b:m:s:Cvdh";
extern char *optarg;

/*
 * Algorithm parameters.
 */
struct devol_params algo_params = {

  /*
   * Algorithm parameters.
   */
  .gene_dispersal_factor = 0.0,
  .reproduction_rate = .25,
  .breed_fitness = .25,
  .rstate = {7, 20, 1969}, 

  /*
   * These are the functions used to actually run the algorithm.
   */
  .mutate  = mutate,
  .fitness = fitness,
  .init    = init,
  .destroy = destroy,
  .swap    = swap,

  /* And some default random state. */
  .rstate = {7, 20, 1969},

};

/*
 * Memory allocation for the solutions. Lockless, fast, etc. Requires two
 * allocators since each allocator is only capableof allocating fixed sized
 * chunks.
 */
struct bucket_table mix_sols;
struct bucket_table mix_params;

/*
 * The list of normals and the list of data.
 */
struct normal *norms;
int            norms_len;
double        *samples;
int            sample_count;

/*
 * main(). Start here...
 */
int main(int argc, char **argv){

  int i;
  char arg;
  char *not_ok;

  int *rng_seed;
  int elems;

  time_t t_start;
  time_t t_stop;
  struct timeb tmp_time;

  /* Parse the args. */
  while ( (arg = getopt_long(argc, argv, args, mix_opts, NULL)) != -1 ){

    switch (arg){

    case 'd':
      data_file = strdup(optarg);
      break;
    case 'n':
      norms_file = strdup(optarg);
      break;
    case 'p': /* population size */
      pop_size = (int) strtol(optarg, &not_ok, 0);
      if ( *not_ok )
	die("Unable to parse population size.\n");
      break;
    case 'r': /* reproduction rate */
      algo_params.reproduction_rate = strtod(optarg, &not_ok);
      if ( *not_ok )
	die("Unable to parse reproduction rate.\n");
      break;
    case 'D': /* gene dispersal */
      algo_params.gene_dispersal_factor = strtod(optarg, &not_ok);
      if ( *not_ok )
	die("Unable to parse reproduction rate.\n");
      break;
    case 't':
      threads = (int) strtol(optarg, &not_ok, 0);
      if ( *not_ok )
	die("Unable to parse thread count.\n");
      break;
    case 'b': /* breed fitness */
      algo_params.breed_fitness = strtod(optarg, &not_ok);
      if ( *not_ok )
	die("Unable to parse breed fitness.\n");
      break;
    case 'm': /* maximum iterations */
      max_iter = (int) strtol(optarg, &not_ok, 0);
      if ( *not_ok )
	die("Unable to parse maximum iterations.\n");
      break;
    case 's': /* random seed data */
      rng_seed = parse_integer_array(optarg, &elems);
      if ( elems != 3 ){
	die("Please use 3 integer shorts for the RNG seed.\n");
      }
      algo_params.rstate[0] = rng_seed[0];
      algo_params.rstate[1] = rng_seed[1];
      algo_params.rstate[2] = rng_seed[2];
      printf("Setting random seed: [%u,%u,%u]\n", 
	     rng_seed[0], rng_seed[1], rng_seed[2]);
      free(rng_seed);
      break;
    case 'C': /* We should check for convergence. */
      converge = 1;
      break;
    case 'v': /* We should be verbose. */
      verbose = 1;
      break;
    case 'h': /* Print a help message. */
      help = 1;
      break;
    case 0:   /* a long opt with no arg was parsed */
      break;
    case '?': /* an error was encountered */
      fprintf(stderr, "Error parsing arguments.\n");
      exit(1);

    }
  }

  if ( ! data_file )
    die("You must specify a data file.\n");
  if ( ! norms_file )
    die("You must specify a norms file.\n");
  
  printf("# Algorithm parameters:\n");
  printf("#   Population size:      %d\n", pop_size);
  printf("#   Thread count:         %d\n", threads);
  printf("#   Maximum iterations:   %d\n", max_iter);
  printf("#   Gene dispersal:       %lf\n", algo_params.gene_dispersal_factor);
  printf("#   Reproduction rate:    %lf\n", algo_params.reproduction_rate);
  printf("#   Breed fitness:        %lf\n", algo_params.breed_fitness);
  printf("#   Check for converge:   %s\n", converge ? "yes" : "no");
  printf("#   Data file:            %s\n", data_file);
  printf("#   Normal distributions: %s\n", norms_file);

  /* Here is the real start of essential algorithm, everything else is just
   * printf()s and arg validation which is meh. */
  ftime(&tmp_time);
  t_start = (tmp_time.time * 1000) + tmp_time.millitm;

  /* Read in the normals. */
  norms = read_mixture_file(norms_file, &norms_len);
  printf("# Read %d normal distributions.\n", norms_len);
  for ( i = 0; i < norms_len; i++){
    printf("#   %s: mean=[%.4lf,%.4lf] stddev=[%.4lf,%.4lf]"
	   " var=[ %.4lf %.4lf ]\n", norms[i].name,
	   norms[i].mu_min, norms[i].mu_max,
	   norms[i].sigma_min, norms[i].sigma_max,
	   norms[i].mu_var, norms[i].sigma_var);
  }

  /* Read in the data. */
  samples = read_data_file(data_file, &sample_count);
  printf("# Read %d data samples.\n", sample_count);

  /* Now prepare to run the algorithm. */
  run();

  /* And the end of the algorithm run time... */
  ftime(&tmp_time);
  t_stop = (tmp_time.time * 1000) + tmp_time.millitm;

  printf("run time: %ld ms\n", t_stop - t_start);

  return 0;

}

int run(){

  int i;
  int iter = 0;
  int err;
  int blocks;
  struct gene_pool pool;

  /* Initialize the solution's memory allocator. */
  blocks = algo_params.reproduction_rate * pop_size;
  blocks *= 2;        /* Just for good measure. */
  blocks += pop_size; /* The steady state solutions. */
  blocks /= threads;  /* Since we split each bucket by thread. */
  printf("# Initializing mixture allocation buckets.\n");
  init_bucket_allocator(&mix_sols, threads, sizeof(struct mixture_solution),
			blocks);
  /*_display_buckets(&mix_sols, 0);*/
  printf("# Initializing param allocation buckets.\n");
  init_bucket_allocator(&mix_params, threads, sizeof(double) * 3 * norms_len,
			blocks);
  /*_display_buckets(&mix_params, 0);*/

  /* Initialize the gene pool. */
  err = gene_pool_create(&pool, pop_size, threads, algo_params);
  if ( err )
    die("Unable to initialize the gene pool :(.\n");

  if ( verbose )
    for ( i = 0; i < pop_size; i++)
      print_solution(&pool.solutions[i]);

  printf("# Gene pool made, solutions inited, running...\n");
  
  /* Run the algorithm. */
  while ( iter++ < max_iter ){

    gene_pool_iterate(&pool);
    
    /* Print the average fitness of the solution pool. */
    if ( converge )
      printf("%6d\t%lf\n", iter, gene_pool_avg_fitness(&pool));

  }

  if ( verbose )
    for ( i = 0; i < pop_size; i++)
      print_solution(&pool.solutions[i]);

  /* We are done... */
  return 0;

}

int cross_over(solution_t *par1, solution_t *par2, solution_t *dest){

  int i;
  long int cpoint;
  struct devol_controller *cont = par1->cont;
  struct mixture_solution *m1, *m2, *ds;

  m1 = par1->private.ptr;
  m2 = par2->private.ptr;
  ds = dest->private.ptr;

  /*
   * Here lies the code that allows us to not use cross over just to see the
   * difference in average solution fitness over time with or with out cross
   * over.
   */
  /*
  if ( m1->mle > m2->mle){
    for ( i = 0; i < norms_len; i++){
      ds->mu[i] = m1->mu[i];
      ds->sigma[i] = m1->sigma[i];
      ds->prob[i] = m1->prob[i];
    }
  } else {
    for ( i = 0; i < norms_len; i++){
      ds->mu[i] = m2->mu[i];
      ds->sigma[i] = m2->sigma[i];
      ds->prob[i] = m2->prob[i];
    }
  }  
  return 0;
  */

  /* Pick a random number less than the number of distributions we are using.
   * Then take params from parent 1 until we hit the crossover point; then
   * take params from the other parent. */
  nrand48_r(cont->rstate, &cont->rdata, &cpoint);
  cpoint %= norms_len;

  /* OK, we have a crossover point. Now make the child. */
  for ( i = 0; i < norms_len; i++){
    ds->mu[i] = (i < cpoint) ? m1->mu[i] : m2->mu[i];
    ds->sigma[i] = (i < cpoint) ? m1->sigma[i] : m2->sigma[i];

    /* We cant crossover probabilities. This leads to huge problems. */
    ds->prob[i] = m1->prob[i];
  }

  return 0;

}

int mutate(solution_t *par1, solution_t *par2, solution_t *dest){

  int i;
  long int p_plus, p_minus;
  double d_mu, d_sigma, d_prob;
  
  struct mixture_solution *ms;
  struct devol_controller *cntr = par1->cont;

  /* We are passed a pair of solutions. Make a third from those two. First init
   * the solution, then perform some crossover, then finally, randomly perturb
   * the child solution. */
  init(dest);
  ms = dest->private.ptr;
  cross_over(par1, par2, dest);
  //printf("# Crossover:\n");
  //print_solution(dest);

  /* Do the random perturbations here. */
  for ( i = 0; i < norms_len; i++){

    /* Get the initial randoms. */
    devol_rand48(cntr->rstate, &cntr->rdata, &d_mu);
    devol_rand48(cntr->rstate, &cntr->rdata, &d_sigma);

    /* Now fit them into the variance window. */
    d_mu = (d_mu * norms[i].mu_var) - (norms[i].mu_var/2);
    d_sigma = (d_sigma * norms[i].sigma_var) - (norms[i].sigma_var/2);

    /* Add the changes in. */
    ms->mu[i] += d_mu;
    ms->sigma[i] += d_sigma;

  }

  /* We do one probability modification per iteration for simplicity's sake. */
  if ( norms_len > 1 ){
    devol_rand48(cntr->rstate, &cntr->rdata, &d_prob);  
    jrand48_r(cntr->rstate, &cntr->rdata, &p_plus);
    d_prob = (d_prob * PROB_VAR) - (PROB_VAR/2);
    p_plus = labs(p_plus);
    p_plus %= norms_len;
    
    do {
      jrand48_r(cntr->rstate, &cntr->rdata, &p_minus);
      p_minus = labs(p_minus);
      p_minus %= norms_len;
    } while ( p_minus == p_plus );

    /* The probability has to sum to 1 after all. */
    ms->prob[p_plus]  += d_prob;
    ms->prob[p_minus] -= d_prob;

  }

  //printf("# Perturbations:\n");
  //print_solution(dest);

  return 0;

}

#define ONE_DIV_ROOT_2_PI 0.39894

/*
 * The normal PDF function. Calculated with respect to the unit normal 
 * (sigma=1 and mu=0). To use diff normal params, shift the passed argument 
 * correctly.
 */
double _normal_pdf(double x){

  return ONE_DIV_ROOT_2_PI * exp(-( .5 * x * x ));

}

/*
 * Calculate the maximum likelihood of the passed point with respect to the
 * normal distributions of the passed solution.
 *
 * The normal PDF:
 *
 *   ( 1 / abs(ROOT_2_PI*sigma) ) * exp( -(x-mu)^2 / 2*sigma^2 )
 *
 */
double _do_mle_point_estimate(struct mixture_solution *s, double x){

  int i;

  double sum = 0.0;
  double samp;

  /* For each normal distribution: */
  for ( i = 0; i < norms_len; i++){

    /* Calculate the value of the normal PDF for the params. */
    samp = _normal_pdf( (x - s->mu[i]) / s->sigma[i] ) / s->sigma[i];

    /* And add it into the sum. */
    sum += s->prob[i] * samp;

  }

  /* Return the sum; which is now the MLE for the passed data point. */
  //printf("# %lf -> %lf (exp=%lf scale=%lf\n", x, sum, exponent, scale);
  return sum;

}

/*
 * Calculate the maximum likelihood function for the passed parameters. For
 * each data point, calculate the sum of the weighted normal log probability.
 */
double fitness(solution_t *solution){

  int i;
  
  double mle;
  double fitness = 0.0;

  struct mixture_solution *ms = solution->private.ptr;

  if ( ms->solved )
    return ms->mle;

  /* For each data point, calculate the MLE estimate. Then take the log, and
   * finally add it into our fitness value. */
  for ( i = 0; i < sample_count; i++){

    mle = _do_mle_point_estimate(ms, samples[i]);
    fitness += log(mle);

  }

  /* Since we want a value close to zero, we simply take an arbitrary ceiling
   * value for the fitnees, and return the distance the computed MLE is from
   * that fitness. */
  ms->solved = 1;
  ms->mle = FITNESS_CEILING - fitness;
  return ms->mle;

}

/*
 * Initialize a solution to hold a random guess as to what the mixture of
 * distributions will be. We assume that each solution passed already has its
 * functions set up properly, so we can ignore that part of initialization.
 */
int init(solution_t *solution){

  int i;
  double tmp = 0;
  double mu, sigma;
  struct mixture_solution *msol;
  struct devol_controller *cont = solution->cont;

  msol = (struct mixture_solution *)balloc(&mix_sols, cont->tid);
  if ( ! msol )
    die("init solution: out of memory.\n");

  /* All the sigma, mu, and prob allocations in 1 operation. */
  msol->mu = (double *)balloc(&mix_params, cont->tid);
  msol->sigma = msol->mu + norms_len;
  msol->prob = msol->mu + (2 * norms_len);
  msol->solved = 0;

  msol->len = norms_len;
  for ( i = 0; i < norms_len; i++){
    /* Generate a random number on the mu interval. */
    devol_rand48(cont->rstate, &cont->rdata, &tmp);
    mu = (tmp * (norms[i].mu_max - norms[i].mu_min)) + norms[i].mu_min;

    /* Generate a random number on the sigma interval. */
    devol_rand48(cont->rstate, &cont->rdata, &tmp);
    sigma = 
      (tmp * (norms[i].sigma_max - norms[i].sigma_min)) + norms[i].sigma_min;
    
    /* And set the msol fields. */
    msol->mu[i] = mu;
    msol->sigma[i] = sigma;
    msol->prob[i] = 1.0 / norms_len;

  }

  solution->private.ptr = msol;

  return 0;

}

/*
 * Free the buffers held by the passed solution.
 */
int destroy(solution_t *solution){

  struct mixture_solution *sol = solution->private.ptr;

  /* Since mu and sigma were made in one allocation, they must be destroyed in
   * one free(). */
  bfree(&mix_params, solution->cont->tid, sol->mu);

  /* And free the solution itself. */
  bfree(&mix_sols, solution->cont->tid, sol);

  return 0;

}

/*
 * Deep swap of two solutions. Necessary because the memory allocator I wrote
 * is really, really, finicky. Gah this is computationally expensive :(. Thats
 * a downside to the shitty memory allocator I wrote I guess. Maybe I should
 * that allocator able to do frees w/o knowing the bucket a memory block is
 * from.
 */
void swap(solution_t *left, solution_t *right){

  int t_solved;
  double tmp;
  double t_theta[norms_len * 3];
  struct mixture_solution *l_sol = left->private.ptr;
  struct mixture_solution *r_sol = right->private.ptr;

  /* Copy the actual values of the parameters; leave the original addresses of
   * each of the mixture structs alone so we dont break the memory allocator.
   */
  memcpy(t_theta,   l_sol->mu, sizeof(double) * norms_len * 3);
  memcpy(l_sol->mu, r_sol->mu, sizeof(double) * norms_len * 3);
  memcpy(r_sol->mu, t_theta,   sizeof(double) * norms_len * 3);

  /* Now that the parameters themselves are copied, we should probably copy
   * some of the other fields as well. */
  tmp                = left->fitness_val;
  left->fitness_val  = right->fitness_val;
  right->fitness_val = tmp;
  tmp                = l_sol->mle;
  l_sol->mle         = r_sol->mle;
  r_sol->mle         = tmp;
  t_solved           = l_sol->solved;
  l_sol->solved      = r_sol->solved;
  r_sol->solved      = t_solved;

}

/*
 * Parse a comma seperated list of integers.
 */
int *parse_integer_array(char *list, int *count){

  int i;
  int length = 0;
  size_t list_len = strlen(list);

  int *ilist;
  char *current, *next;

  /* First count the number of commas, this + 1 is the number of doubles
   * we expect (assuming the list isn't terminated by a comma). */
  for ( i = 0; i < list_len; i++){
    if ( list[i] == ',' )
      length++;
  }
  length++;

  /* Allocate memory. */
  ilist = (int *)malloc(sizeof(int) * length);
  if ( ! ilist )
    return NULL;
  memset(ilist, 0, sizeof(int) * length);

  /* Now we must iterate across this list and parse some doubles. */
  current = list;
  next = list;
  i = 0;
  while ( *next != 0 ){

    /* Do the conversion and make sure it went ok. */
    ilist[i++] = (int)strtol(current, &next, 0);
    if ( *next != 0 && *next != ',' ){
      fprintf(stderr, "Error parsing: %s\n", list);
      free(ilist);
      return NULL;
    }
    
    /* Now set current to 1 position past next (to skip the commas). */
    current = next+1;

  }

  *count = length;
  return ilist;

}

/*
 * Print an error message and quit.
 */
void die(char *msg){

  fprintf(stderr, msg);
  exit(1);

}

/*
 * Print a solution out. In a pretty way.
 */
void print_solution(solution_t *s){

  int i;
  struct mixture_solution *ms = s->private.ptr;

  printf("# Solution: (fitness = %lf)\n", ms->mle);
  for ( i = 0; i < ms->len; i++){
    printf("#  mu = %.4lf sigma = %.4lf prob = %.4lf\n", 
	   ms->mu[i], ms->sigma[i], ms->prob[i]);
  }

}
