/*
 * Solves roots of polynomials. Obviously this can be solved with much more
 * specific algorithms but the goal is to show functionality of an evolutionary
 * approach. This program is executed sequentially. 
 *
 * Relevant paramaters:
 *
 *   coeff         <a0,a1,a2, ...,an>   The coefficients to a polynomial in the
 *                                      form a0 + a1*X + a2*X^2 + ... + an*X^n
 *   x_min         <double>             The minimum starting search bound.
 *   x_max         <double>             The maximum starting search bound.
 *   pop-size      <integer>            The population size.
 *   rep-rate      <double>             The reproduction rate of the pop.
 *   breed-fitness <double>             Percent of the population that is
 *                                      allowed to breed.
 *   max-iter      <integer>            Maximum iterations.
 *   variance      <double>             How much to vary each solution when it
 *                                      is bred.
 *   seed          <s1,s2,s3>           3 unsigned short integer seed values
 *                                      for the drand48() family of random 
 *                                      functions.
 *   converge      N/A                  If specified terminate the algorithm
 *                                      when the average population fitness is
 *                                      less than variance.
 *   verbose       N/A                  Will be verbose.
 *   defaults      N/A                  Print the default values for the 
 *                                      variables w/ defaults.
 *   help          N/A                  Display a help message.
 *
 * 'coeff' should be specified as a comma seperated list of values. An example:
 *   ./root_finder --coeff=-3,0,1 --pop-size=200 --max-iter=1000 --converge
 *
 * This incantation will find the roots of -3*(X^2)+1 with a population of 200
 * and a maximum iteration count of 1000. It will also terminate and print the
 * number of iterations for convergence, if convergence happens.
 * 
 */

#include <devol.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>

/*
 * Prototypes for functions we need.
 */
int     mutate(solution_t *par1, solution_t *par2, 
	       solution_t *dest);
double  fitness(solution_t *solution);
int     init(solution_t *solution);
int     destroy(solution_t *solution);
double *parse_double_array(char *list, int *count);
int    *parse_integer_array(char *list, int *count);
void    die(char *msg);
int     run();

/*
 * Fields that modify the functionality of the program.
 */
int converge = 0;
int verbose  = 0;
int defaults = 0;
int help     = 0;

/*
 * Default fields that define the behavior of this algorithm.
 */
double *coeffs;
int    num_coeffs;
double x_min = -1.0;
double x_max =  1.0;
int    pop_size = 100;
int    max_iter = 100;
double variance = .001;

struct devol_params algo_params = {

  /*
   * Only relevant to multithreaded algorithms.
   */
  .gene_dispersal_factor = 0.0,

  /*
   * Specify the number of new solutions based on a percent of the original
   * population. 0.0 = no reproduction (this isn't a good idea), and 1.0 tells
   * the algorithm to make a new solution fo every solution in the starting
   * population. Default is 1/4 of the original population size new solutions.
   */
  .reproduction_rate = .25,

  /*
   * This paramater specifies how fit a solution must be relative to the
   * population in order for it to breed. This paramater varies between 0.0 and
   * 1.0 such that, if breed_fitness = 0.0, then no solutions will be allowed
   * to breed, and if breed_fitness = 1.0, then all solutions will be allowed
   * to breed. Default is the top 25% most fit solutions may breed. 
   */
  .breed_fitness = .25,

  /*
   * Some random state to start the algorithm.
   */
  .rstate = {7, 20, 1969}, 

  /*
   * These are the functions used to actually run the algorithm.
   */
  .mutate  = mutate,
  .fitness = fitness,
  .init    = init,
  .destroy = destroy,

};

/*
 * These are the arguements themselves.
 */
struct option rf_opts[] = {
  
  {"coeff", 1, NULL, 'c'},
  {"x-min", 1, NULL, 'N'},
  {"x-max", 1, NULL, 'X'},
  {"pop-size", 1, NULL, 'p'},
  {"rep-rate", 1, NULL, 'r'},
  {"breed-fitness", 1, NULL, 'b'},
  {"max-iter", 1, NULL, 'm'},
  {"variance", 1, NULL, 'V'},
  {"seed", 1, NULL, 's'},
  {"converge", 0, &converge, 'C'},
  {"verbose", 0, &verbose, 'v'},
  {"defaults", 0, &defaults, 'd'},
  {"help", 0, &help, 'h'},
  {NULL, 0, NULL, 0},

};
char *args = "c:N:X:p:r:b:m:V:s:Cvdh";
extern char *optarg;


int main(int argc, char **argv){

  int i;
  char arg;
  char *not_ok;

  int *rng_seed;
  int elems;

  /*
   * Start parsing the arguements.
   */
  while ( (arg = getopt_long(argc, argv, args, rf_opts, NULL)) != -1 ){

    switch (arg){

    case 'c': /* coeff */
      coeffs = parse_double_array(optarg, &num_coeffs);
      if ( ! coeffs )
	die("Unable to parse coefficients.\n");
      break;
    case 'N': /* x-min */
      x_min = strtod(optarg, &not_ok);
      if ( *not_ok )
	die("Unable to parse x-min.\n");
      break;
    case 'X': /* x-max */
      x_max = strtod(optarg, &not_ok);
      printf("*not_ok: %d\n", *not_ok);
      if ( *not_ok )
	die("Unable to parse x-max.\n");
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
    case 'V': /* variance */
      variance = strtod(optarg, &not_ok);
      if ( *not_ok )
	die("Unable to parse solution variance.\n");
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
    case 'd': /* Print the default values for relevant paramaters. */
      defaults = 1;
      break;
    case 0:   /* a long opt with no arg was parsed */
      break;
    case '?': /* an error was encountered */
      fprintf(stderr, "Error parsing arguments.\n");
      exit(1);

    }

  }

  if ( num_coeffs < 1 ){
    die("You must specify some coefficients.\n");
  }

  printf("Solving polynomial with coefficients:\n  [");
  for ( i = 0; i < num_coeffs-1; i++){
    printf(" %lf", coeffs[i]);
  }
  printf(" %lf ]\n", coeffs[i]);

  printf("Algorithm parameters:\n");
  printf("  Population size:        %d\n", pop_size);
  printf("  Solution variance:      %lf\n", variance);
  printf("  Initial solution range: [%lf,%lf]\n", x_min, x_max);
  printf("  Maximum iterations:     %d\n", max_iter);
  printf("  Reproduction rate:      %lf\n", algo_params.reproduction_rate);
  printf("  Breed fitness:          %lf\n", algo_params.breed_fitness);
  printf("  Check for converge:     %s\n", converge ? "yes" : "no");

  /* OK, we are now ready to begin. */
  run();

  return 0;

}

/*
 * Run the algorithm.
 */
int run(){

  int i;
  int iterations = 0;
  double avg_fitness;
  struct gene_pool seq_pool;

  gene_pool_create_seq(&seq_pool, pop_size, algo_params);

  if ( verbose ){
    printf("Initial population:\n");
    gene_pool_display_fitnesses(&seq_pool);
  }

  while ( iterations++ < max_iter ){

    gene_pool_iterate_seq(&seq_pool);

    if ( converge ){
      avg_fitness = gene_pool_avg_fitness(&seq_pool);
      if ( avg_fitness <= variance ){
	printf("Convergence after %d iterations: avg fitness=%lf\n",
	       iterations, avg_fitness);
	break;
      } else {
	printf("Iteration (%d): %lf\n", iterations, avg_fitness);
      }
    }

  }

  /* Print the solution pool at the end of the run if verbose is set. */
  if ( verbose ){
    for ( i = 0; i < pop_size; i++){
      printf("Solution %6d: X = %-10lf fitness = %-10lf\n", i+1,
	     seq_pool.solutions[i].private.dp_fp, 
	     seq_pool.solutions[i].fitness_val);
    }	 
  }

  return 0;

}

/*
 * Very simple. Just modify the X value by a small amount.
 */
int mutate(solution_t *par1, solution_t *par2, 
	   solution_t *dest){

  double tmp;
  double base;
  double variation;

  /* Pick the better solution of the two and then vary it by a little bit. */
  if ( par1->fitness_val >= par2->fitness_val )
    base = par1->private.dp_fp;
  else
    base = par2->private.dp_fp;

  /* And vary it by a little bit. */
  devol_rand48(par1->cont->rstate, &(par1->cont->rdata), &tmp);
  variation = (tmp * variance) - (variance/2);

  /* Initialize and set the destination solution. */
  dest->private.dp_fp = base + variation;  

  return 0;

}

/*
 * Compute the polynomial for the passed solution's x value.
 */
double fitness(solution_t *solution){

  int i;
  double x = solution->private.dp_fp;
  double power = 1;
  double sum = 0.0;
  
  for ( i = num_coeffs-1; i >= 0; i--){
    sum += (coeffs[i] * power);
    power *= x;
  }

  return fabs(sum);

}

/*
 * Generate a solution randomly on the interval [x_min,x_max].
 */
int init(solution_t *solution){

  double sol;

  /* This gets a random number in the same window size as [x_min,x_max]. Then
   * scale it to the correct offset by subtracting x_min. */
  sol = erand48(algo_params.rstate) * (x_max - x_min);
  sol += x_min;

  solution->private.dp_fp = sol;

  return 0;

}

int destroy(solution_t *solution){

  return 0;

}

/*
 * Parse a comma seperated list of doubles.
 */
double *parse_double_array(char *list, int *count){

  int i;
  int length = 0;
  size_t list_len = strlen(list);

  double *dlist;
  char *current, *next;

  /* First count the number of commas, this + 1 is the number of doubles
   * we expect (assuming the list isn't terminated by a comma). */
  for ( i = 0; i < list_len; i++){
    if ( list[i] == ',' )
      length++;
  }
  length++;

  /* Allocate memory. */
  dlist = (double *)malloc(sizeof(double) * length);
  if ( ! dlist )
    return NULL;
  memset(dlist, 0, sizeof(double) * length);

  /* Now we must iterate across this list and parse some doubles. */
  current = list;
  next = list;
  i = 0;
  while ( *next != 0 ){

    /* Do the conversion and make sure it went ok. */
    dlist[i++] = strtod(current, &next);
    if ( *next != 0 && *next != ',' ){
      fprintf(stderr, "Error parsing: %s\n", list);
      free(dlist);
      return NULL;
    }
    
    /* Now set current to 1 position past next (to skip the commas). */
    current = next+1;

  }

  *count = length;
  return dlist;

}

/*
 * Parse a comma seperated list of integers. This is a copy of the code in 
 * parse_double_array :(. Pretty much has to be done this way since I can't
 * substitute data types in C. Oh well.
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
