/*
 * Do I/O related to the mixture problem.
 */

#include <mixture.h>

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

/* How many normals we allocate at a time. */
#define NORM_INC 3
#define DATA_INC 10000

/*
 * Read in a list of normal distribution paramaters. Return a list of structs
 * that define them for the actual mixture program.
 */
struct normal *read_mixture_file(char *file, int *norms){

  int i = 0;
  int fields;
  int norms_length = 0;
  int norms_max = 0;
  FILE *nfile;
  struct normal *normals = NULL;// *tmp = NULL;
  
  nfile = fopen(file, "r");
  if ( ! nfile ){
    perror("Unable to read mixture file");
    return NULL;
  }
  
  /* Now start reading in norms. */
  while ( 1 ){

    /* Make sure we have enough mem for the next normal. */
    if ( norms_length >= norms_max ){

      /* Get the new memory. */
      if ( ! normals ){
	norms_max = NORM_INC;
	normals = (struct normal *)malloc(sizeof(struct normal) * norms_max);
	if ( ! normals ){
	  fprintf(stderr, "Out of memory.\n");
	  return NULL;
	}
      } else {
	
	/* Reallocate the array. */
	norms_max += NORM_INC;
	normals = realloc(normals, norms_max);

      }

    }

    fields = fscanf(nfile, "%64s (%lf,%lf) (%lf,%lf) %lf %lf\n", 
		    normals[i].name, 
		    &normals[i].mu_min,    &normals[i].mu_max,
		    &normals[i].sigma_min, &normals[i].sigma_max,
		    &normals[i].mu_var,    &normals[i].sigma_var);

    /* When we don't read in 7 feilds just assume we are done. */
    if ( fields != 7 )
      break;

    /* Otherwise, we have successfully read a normal, so inc our counter. */
    norms_length += 1;
    i++;

  }
 
  *norms = norms_length;
  return normals;

}

/*
 * Read in the data.
 */
double *read_data_file(char *file, int *sample_count){

  int i = 0;
  int fields;
  int data_max = 0;
  int data_length = 0;
  FILE *dfile;
  double *samples = NULL, *tmp = NULL;

  dfile = fopen(file, "r");
  if ( ! dfile ){
    perror("Unable to read mixturesample file");
    return NULL;
  }

  /* Now start reading in norms. */
  while ( 1 ){

    if ( data_length >= data_max ){

      /* Get the new memory. */
      tmp = (double *)malloc(sizeof(double) * DATA_INC);
      if ( ! tmp ){
	fprintf(stderr, "Out of memory.\n");
	if ( samples ) free(samples);
	return NULL;
      }

      /* Copy old mem into the new mem. */
      if ( samples ){
	memcpy(tmp, samples, sizeof(double) * data_length);
	free(samples);
      }
      samples = tmp;
      data_max += NORM_INC;

    }

    fields = fscanf(dfile, "%lf\n", &samples[i]);
    if ( fields != 1 )
      break;

    data_length++;
    i++;

  }

  *sample_count = data_length;
  return samples;

}
