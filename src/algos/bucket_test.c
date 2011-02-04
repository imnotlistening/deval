/*
 * Test out the bucket code. Pretty straight forward.
 */

#include <mixture.h>

#include <stdio.h>

struct bucket_table tbl1;
struct bucket_table tbl2;


int main(){

  /* Init the buckets. */
  init_bucket_allocator(&tbl1, 2, sizeof(struct mixture_solution), 100);
  init_bucket_allocator(&tbl2, 2, sizeof(double) * 3 * 2, 100);

  /* Now start allocating and deallocating from said buckets. */
  _display_buckets(&tbl1, 0);
  _display_buckets(&tbl2, 0);

  /* Do some allocations */
  void *a1 = balloc(&tbl1, 1);
  void *a2 = balloc(&tbl1, 1);
  void *a3 = balloc(&tbl1, 1);
  void *a4 = balloc(&tbl1, 1);

  _display_buckets(&tbl2, 1);

  printf(" a1 = %p\n", a1);
  printf(" a2 = %p\n", a2);
  printf(" a3 = %p\n", a3);
  printf(" a4 = %p\n", a4);

  bfree(&tbl1, 1, a2);
  bfree(&tbl1, 1, a3);

  _display_buckets(&tbl1, 1);
  a2 = balloc(&tbl1, 1);
  _display_buckets(&tbl1, 1);
  

  /* This should be a little more strenuous. */
  int i;
  void *tmp[100];
  for ( i = 0; i < 105; i++){
    tmp[i] = balloc(&tbl1, 1);
    if ( ! tmp[i] ){
      printf("Error on allocation %d\n", i+1);
      break;
    }
  }

  _display_buckets(&tbl1, 1);

  for ( i = 0; i < 100; i++)
    bfree(&tbl1, 1, tmp[i]);
  bfree(&tbl1, 1, a1);
  bfree(&tbl1, 1, a2);
  bfree(&tbl1, 1, a4);

  _display_buckets(&tbl1, 1);

  return 0;

}
