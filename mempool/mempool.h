#ifndef MEMPOOL_H
#define MEMPOOL_H
#include <stdint.h>
#include <pthread.h>

typedef enum {
  FALSE,
  TRUE
} boolean;

/*
* NAME :        mmblockhead_s
*
* DESCRIPTION : To store block headers
*
* MEMBERS :     used - Flag to show memory is not free
*               prevp - Pointer to next block
*               nextp - Pointer to next block
*
* NOTES :      None
*/
struct mmblockhead_s
{
  boolean used;
  struct mmblockhead_s *prevp;
  struct mmblockhead_s *nextp;
};

/*
* NAME :        mempool_t
*
* DESCRIPTION : Pool control block
*
* MEMBERS :     poolinited - flag for initialization
*               memusedp - Pointer to used link list
*               memfreedp - Pointer to freed link list
*               membasep - Pointer to memory base address
*               objsize - User object size
*               blksize - Memory block size
*               numblk - Number of blocks
*               totalsize - Pool total size
*               mutex - Locking mechanism
*
* NOTES :      None
*/
typedef struct
{
  boolean poolinited;
  struct mmblockhead_s *memusedp;
  struct mmblockhead_s *memfreedp;
  uint8_t *membasep;
  uint32_t objsize;
  uint32_t blksize; /* number of bytes in each block */
  uint32_t numblk;  /* number of blocks in the pool */
  uint32_t totalsize;
  pthread_mutex_t mutex;
} mempool_t;


extern boolean mempool_init(
  mempool_t *poolp,
  uint32_t num_blocks,
  uint32_t block_size);

extern void mempool_destroy(mempool_t *poolp);

extern void *mempool_alloc(mempool_t *poolp);

extern boolean mempool_rel(
  mempool_t *poolp,
  void *memp);

extern boolean mempool_is_mem_valid(
  mempool_t *poolp,
  void *memp);

void mempool_print_stat(
  mempool_t *poolp);
#endif