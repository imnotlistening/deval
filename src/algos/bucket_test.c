/*
 * Test out the bucket code. Pretty straight forward.
 */

#include <mixture.h>

#include <stdio.h>

struct bucket_table tbl1;
struct bucket_table tbl2;


int main(){

  /* Init the buckets. */
  init_bucket_allocator(&tbl1, 4, sizeof(struct mixture_solution), 10);
  init_bucket_allocator(&tbl1, 4, sizeof(double) * 3 * 2, 10);

  /* Now start allocating and deallocating from said buckets. */
  

  return 0;

}
