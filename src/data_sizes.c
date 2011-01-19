/*
 * Print out sizes of relevant data type sizes.
 */

#include <devol.h>

#include <stdio.h>

int main(){

#ifdef __x86_64__
  printf("sizeof(solution_t): %ld\n", sizeof(solution_t));
  printf("sizeof(struct gene_pool): %ld\n", sizeof(struct gene_pool));
  printf("sizeof(struct devol_params): %ld\n", sizeof(struct devol_params));
  printf("sizeof(struct thread_pool): %ld\n", sizeof(struct thread_pool));
  printf("sizeof(struct devol_controller): %ld\n", 
	 sizeof(struct devol_controller));
#else
  printf("sizeof(solution_t): %d\n", sizeof(solution_t));
  printf("sizeof(struct gene_pool): %d\n", sizeof(struct gene_pool));
  printf("sizeof(struct devol_params): %d\n", sizeof(struct devol_params));
  printf("sizeof(struct thread_pool): %d\n", sizeof(struct thread_pool));
  printf("sizeof(struct devol_controller): %d\n", 
	 sizeof(struct devol_controller));
#endif


  return 0;

}
