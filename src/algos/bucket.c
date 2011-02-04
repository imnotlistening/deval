/*
 * This is a very specific implementation of a memory allocator. The goal is to
 * take an initial memory space and break it up into blocks. These blocks can
 * then be allocated out as necessary. This requires three things: A) you know
 * exactly how big each block must be, B) you know exactly how many blocks you
 * need at most, and C) each block *must* be the same size. Given these
 * constraints, the allocator can be made to be A) multithread safe without
 * requiring any locking, B) concurrent, and C) fast, D) simple. These are all
 * very good things. Yay.
 */

#include <mixture.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Definitions for local functions. */
void _twiddle_block_bit(struct bucket *bkt, unsigned int offset);

/* A useful macro to read a bit from a position in 32 bits. */
#define READ_BIT(BLOCK, OFFSET)	((BLOCK & (1<<OFFSET)) >> OFFSET)

/*
 * Initialize a bucket allocator with the given parameters. Allocate out all
 * the underlying memory, etc, etc.
 */
int init_bucket_allocator(struct bucket_table *tbl, int buckets,
			   size_t block_size, size_t elems){

  int i;
  int alloc_tbl_len;

  tbl->bucket_count = buckets;
  tbl->block_size = block_size;

  /* Make some buckets. */
  tbl->buckets = (struct bucket *)malloc(sizeof(struct bucket) * buckets);
  if ( ! tbl->buckets )
    return -1;

  tbl->base = malloc(buckets * block_size * elems);
  if ( ! tbl->base ){
    free(tbl->buckets);
    return -1;
  }

  /* Compute the number of uint32_t's to hold enough bits so that each bit
   * represents whether or not a block has been allocated. Zer out the table
   * since obviously nothing has been allocated yet. */
  alloc_tbl_len = elems / 32;
  if ( elems % 32 )
    alloc_tbl_len++;
  tbl->alloc_tables = (uint32_t *)malloc(4 * alloc_tbl_len * buckets);
  if ( ! tbl->alloc_tables ){
    free(tbl->buckets);
    free(tbl->base);
    return -1;
  }
  memset(tbl->alloc_tables, 0, 4 * alloc_tbl_len * buckets);

  /* OK, we have some memory. Compute the address of each buckets memory space
   * and the required memory for holding the bucket tracking bits. */
  for ( i = 0; i < buckets; i++){

    tbl->buckets[i].base_addr = tbl->base + (i * block_size * elems);
    tbl->buckets[i].alloc_table = tbl->alloc_tables + (i * alloc_tbl_len);
    tbl->buckets[i].elems = elems;
    tbl->buckets[i].tbl = tbl;

    /*
    printf("# table %2d: elems=%lu base=%p alloc_table=%p (len=%d)\n",
	   i, tbl->buckets[i].elems,
	   tbl->buckets[i].base_addr, tbl->buckets[i].alloc_table,
	   alloc_tbl_len);
    */
  }
  tbl->elems_per_bkt = elems;

  return 0;

}

/*
 * Flip a bit in the passed bkt allocation table.
 */
void _twiddle_block_bit(struct bucket *bkt, unsigned int offset){

  unsigned int block;
  unsigned int bit;
  uint32_t position;
  uint32_t *tbl;

  block = offset / 32; /* Gets us which uint32_t we will find our bit in. */
  bit = offset % 32; /* The bit position on the uint32_t. */
  position = (1<<bit); /* A 1 in the position of our bit. */
  tbl = &(bkt->alloc_table[block]);

  /* And the twiddle. */
  *tbl = ((*tbl & ~position) | position) & ~(*tbl & position);

}

/*
 * The knitty gritty of the allocation.
 */
void *_balloc(struct bucket *bkt){

  int idx = 0;
  int bit_offset = 0;
  void *addr;

  /* Search for the first non-0xFFFFFFFF uint32_t in the allocation table. This
   * is where our first available block should be. */
  while ( bkt->alloc_table[idx] == 0xFFFFFFFF ) idx++;

  /* Now find the first non-zero bit. */
  while ( bit_offset < 32 ){
    if ( ! READ_BIT(bkt->alloc_table[idx], bit_offset) )
      break;
    bit_offset++;
  }

  /* Now we have a bit offset and a table index. Caluclate whether this bit
   * is even in the bucket at all. */
  if ( bit_offset + (idx * 32) >= bkt->elems )
    return NULL;

  /* Now compute the actual address in memory of the found block, twiddle its
   * bit, and finally return the address of the block. */
  addr = bkt->base_addr + ((bit_offset + (idx * 32)) * bkt->tbl->block_size);
  _twiddle_block_bit(bkt, bit_offset + (idx * 32));

  return addr;

}

void *balloc(struct bucket_table *tbl, int bucket){

  /* First make sure we have a valid bucket. */
  if ( bucket < 0 || bucket >= tbl->bucket_count )
    return NULL;

  /* Pass of the work to our subordinate... */
  return _balloc(&tbl->buckets[bucket]);

}

void _do_bfree(struct bucket *bkt, void *ptr){

  int block;
  unsigned long int base, addr;
  unsigned long int offset;

  /* Given the ptr, we must figure out what block it points to. Then make sure
   * that block is set to free. If the ptr doesn't point to anything just
   * silently fail. */
  
  base = (unsigned long int) bkt->base_addr;
  addr = (unsigned long int) ptr;
  offset = addr - base;
  
  if ( offset % bkt->tbl->block_size != 0 )
    return;

  /* This is the bit index into the allocation table. */
  block = offset / bkt->tbl->block_size;
  
  /* If that bit is set, then twiddle it, otherwise just return. */
  if ( READ_BIT(bkt->alloc_table[block / 32], block % 32) )
    _twiddle_block_bit(bkt, block);
}

void bfree(struct bucket_table *tbl, int bucket, void *ptr){

  /* Pass this call on to the corrent bucket. */
  _do_bfree(&tbl->buckets[bucket], ptr);

}

/*
 * Print out the allocation table for a given bucket.
 */
void _do_display_bucket(struct bucket *bkt){

  printf("#  Allocation table:");

  int bit = 0;

  while ( bit < bkt->elems ){

    if ( bit % 32 == 0 ) /* New line time. */
      printf("\n#   %6d", bit);

    if ( bit % 8 == 0 ) /* Space for readability */
      printf(" ");

    /* Now finally, print the bit. */
    printf("%c", READ_BIT(bkt->alloc_table[bit / 32], bit % 32) ? '1' : '0');

    bit++;

  }
  printf("\n");

}

/*
 * Print out some info related to the buckets.
 */
void _display_buckets(struct bucket_table *tbl, int print_alloc_tables){

  int i;

  /* Some basic info table wide. */
  printf("# Bucket table info:\n");
  printf("#  Buckets: %d\n", tbl->bucket_count);
  printf("#  Block size: %u\n", (unsigned int)tbl->block_size);
  printf("#  Blocks per bucket: %d\n", tbl->elems_per_bkt);
  printf("#  Bucket table address: %p\n", tbl->buckets);
  printf("#  Allocation table address: %p\n", tbl->alloc_tables);

  if ( ! print_alloc_tables )
    return;

  /* Now list info for each bucket. */
  for ( i = 0; i < tbl->bucket_count; i++ ){
    printf("# Bucket %d:\n", i);
    _do_display_bucket(&tbl->buckets[i]);
  }

}

/*
 * Because I feel like it here is some poetry:
 *
 * Once upon a midnight dreary, while I pondered, weak and weary,
 * Over many a quaint and curious volume of forgotten lore,
 * While I nodded, nearly napping, suddenly there came a tapping,
 * As of some one gently rapping, rapping at my chamber door.
 * "'Tis some visitor," I muttered, "tapping at my chamber door —
 *             Only this, and nothing more."
 * 
 * Ah, distinctly I remember it was in the bleak December,
 * And each separate dying ember wrought its ghost upon the floor.
 * Eagerly I wished the morrow; — vainly I had sought to borrow
 * From my books surcease of sorrow — sorrow for the lost Lenore —
 * For the rare and radiant maiden whom the angels name Lenore —
 *             Nameless here for evermore.
 * 
 * And the silken sad uncertain rustling of each purple curtain
 * Thrilled me — filled me with fantastic terrors never felt before;
 * So that now, to still the beating of my heart, I stood repeating,
 * "'Tis some visitor entreating entrance at my chamber door —
 * Some late visitor entreating entrance at my chamber door; —
 *             This it is, and nothing more."
 * 
 * Presently my soul grew stronger; hesitating then no longer,
 * "Sir," said I, "or Madam, truly your forgiveness I implore;
 * But the fact is I was napping, and so gently you came rapping,
 * And so faintly you came tapping, tapping at my chamber door,
 * That I scarce was sure I heard you"— here I opened wide the door; —
 *             Darkness there, and nothing more.
 * 
 * Deep into that darkness peering, long I stood there wondering, fearing,
 * Doubting, dreaming dreams no mortal ever dared to dream before;
 * But the silence was unbroken, and the stillness gave no token,
 * And the only word there spoken was the whispered word, "Lenore?"
 * This I whispered, and an echo murmured back the word, "Lenore!" —
 *             Merely this, and nothing more.
 * 
 * Back into the chamber turning, all my soul within me burning,
 * Soon again I heard a tapping somewhat louder than before.
 * "Surely," said I, "surely that is something at my window lattice:
 * Let me see, then, what thereat is, and this mystery explore —
 * Let my heart be still a moment and this mystery explore; —
 *             'Tis the wind and nothing more."
 * 
 * Open here I flung the shutter, when, with many a flirt and flutter,
 * In there stepped a stately raven of the saintly days of yore;
 * Not the least obeisance made he; not a minute stopped or stayed he;
 * But, with mien of lord or lady, perched above my chamber door —
 * Perched upon a bust of Pallas just above my chamber door —
 *             Perched, and sat, and nothing more.
 * 
 * Then this ebony bird beguiling my sad fancy into smiling,
 * By the grave and stern decorum of the countenance it wore.
 * "Though thy crest be shorn and shaven, thou," I said, "art sure no craven,
 * Ghastly grim and ancient raven wandering from the Nightly shore —
 * Tell me what thy lordly name is on the Night's Plutonian shore!"
 *             Quoth the Raven, "Nevermore."
 * 
 * Much I marveled this ungainly fowl to hear discourse so plainly,
 * Though its answer little meaning— little relevancy bore;
 * For we cannot help agreeing that no living human being
 * Ever yet was blest with seeing bird above his chamber door —
 * Bird or beast upon the sculptured bust above his chamber door,
 *             With such name as "Nevermore."
 * 
 * But the raven, sitting lonely on the placid bust, spoke only
 * That one word, as if his soul in that one word he did outpour.
 * Nothing further then he uttered— not a feather then he fluttered —
 * Till I scarcely more than muttered, "other friends have flown before —
 * On the morrow he will leave me, as my hopes have flown before."
 *             Then the bird said, "Nevermore."
 * 
 * Startled at the stillness broken by reply so aptly spoken,
 * "Doubtless," said I, "what it utters is its only stock and store,
 * Caught from some unhappy master whom unmerciful Disaster
 * Followed fast and followed faster till his songs one burden bore —
 * Till the dirges of his Hope that melancholy burden bore
 *             Of 'Never — nevermore'."
 * 
 * But the Raven still beguiling all my sad soul into smiling,
 * Straight I wheeled a cushioned seat in front of bird, and bust and door;
 * Then upon the velvet sinking, I betook myself to linking
 * Fancy unto fancy, thinking what this ominous bird of yore —
 * What this grim, ungainly, ghastly, gaunt and ominous bird of yore
 *             Meant in croaking "Nevermore."
 * 
 * This I sat engaged in guessing, but no syllable expressing
 * To the fowl whose fiery eyes now burned into my bosom's core;
 * This and more I sat divining, with my head at ease reclining
 * On the cushion's velvet lining that the lamplight gloated o'er,
 * But whose velvet violet lining with the lamplight gloating o'er,
 *             She shall press, ah, nevermore!
 *
 * Then methought the air grew denser, perfumed from an unseen censer
 * Swung by Seraphim whose footfalls tinkled on the tufted floor.
 * "Wretch," I cried, "thy God hath lent thee — by these angels he hath sent thee
 * Respite — respite and nepenthe, from thy memories of Lenore
 * Quaff, oh quaff this kind nepenthe and forget this lost Lenore!"
 *             Quoth the Raven, "Nevermore."
 * 
 * "Prophet!" said I, "thing of evil! — prophet still, if bird or devil! —
 * Whether Tempter sent, or whether tempest tossed thee here ashore,
 * Desolate yet all undaunted, on this desert land enchanted —
 * On this home by horror haunted— tell me truly, I implore —
 *  Is there — is there balm in Gilead? — tell me — tell me, I implore!"
 *             Quoth the Raven, "Nevermore."
 * 
 * "Prophet!" said I, "thing of evil — prophet still, if bird or devil!
 * By that Heaven that bends above us — by that God we both adore
 * Tell this soul with sorrow laden if, within the distant Aidenn,
 * It shall clasp a sainted maiden whom the angels name Lenore -
 * Clasp a rare and radiant maiden whom the angels name Lenore."
 *             Quoth the Raven, "Nevermore."
 * 
 * "Be that word our sign in parting, bird or fiend," I shrieked, upstarting —
 * "Get thee back into the tempest and the Night's Plutonian shore!
 * Leave no black plume as a token of that lie thy soul hath spoken!
 * Leave my loneliness unbroken!— quit the bust above my door!
 * Take thy beak from out my heart, and take thy form from off my door!"
 *             Quoth the Raven, "Nevermore."
 * 
 * And the Raven, never flitting, still is sitting, still is sitting
 * On the pallid bust of Pallas just above my chamber door;
 * And his eyes have all the seeming of a demon's that is dreaming,
 * And the lamplight o'er him streaming throws his shadow on the floor;
 * And my soul from out that shadow that lies floating on the floor
 *             Shall be lifted — nevermore!
 *
 * The Raven by Edgar Allen Poe
 */

