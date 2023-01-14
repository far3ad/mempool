#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mempool.h"

/*
* NAME :        mempool_init
*
* DESCRIPTION : Creates a memory pool and divides the memory to linked blocks
*
* INPUTS :      poolp - pointer to pool control block
*               num_block - number of blocks
*               block_size - size of each block in bytes
* 
* OUTPUTS :     TRUE - Success
*               FALSE - Failed
*
*  NOTES :      This function is not thread safe.
*/
boolean mempool_init(
  mempool_t *poolp,
  uint32_t num_blocks,
  uint32_t block_size)
{
  uint32_t totalmem;
  struct mmblockhead_s *cur_blkp = NULL;
  uint32_t headsize = sizeof(struct mmblockhead_s);

  if (block_size == 0 || num_blocks == 0 || !poolp)
  {
    printf("%s - Error: Incorrect input parameters.\n", __func__);
    return FALSE;
  }

  /* Set pool to uninitialized */
  poolp->poolinited = FALSE;

  /* Calculate required memory size and allocate memory */
  totalmem = num_blocks * (block_size + headsize);
  poolp->membasep = (uint8_t *) malloc(totalmem);
  if (!poolp->membasep)
  {
    printf("%s - Error: Cannot allocate memory.\n", __func__);
    return FALSE;
  }

  /* Initialized mutex */
  if (pthread_mutex_init(&poolp->mutex, NULL) != 0)
  {
    printf("%s - Error: Cannot initialize mutex.\n", __func__);
    return FALSE;
  }

  /* Initialize parameter */
  memset(poolp->membasep, 0 , totalmem);
  poolp->objsize = block_size;
  poolp->blksize = block_size + headsize;
  poolp->numblk = num_blocks;
  poolp->memfreedp = NULL;
  poolp->memusedp = NULL;
  poolp->totalsize = totalmem;

  /* Initialize blocks */
  for( uint32_t i = 0; i < num_blocks; i++)
  {
    cur_blkp = (struct mmblockhead_s *) (poolp->membasep + i * (block_size + headsize));

    cur_blkp->prevp = NULL;
    cur_blkp->nextp = poolp->memfreedp;
    cur_blkp->used = FALSE;

    if ( NULL != poolp->memfreedp)
    {
      poolp->memfreedp->prevp = cur_blkp;
    }

    poolp->memfreedp = cur_blkp;
  }

  poolp->poolinited = TRUE;

  return TRUE;
}

/*
* NAME :        mempool_destroy
*
* DESCRIPTION : Destroy memory pool
*
* INPUTS :      poolp - pointer to pool control block
*
* OUTPUTS :     None
*
* NOTES :       None
*/
void mempool_destroy(
  mempool_t *poolp)
{
  if (!poolp)
  {
    printf("%s - Error: Invalid pool.\n", __func__);
    return;
  }

  /* Free pool's memory and unset pool's parameters*/
  pthread_mutex_lock(&poolp->mutex);
  if (poolp->poolinited)
  {
    free(poolp->membasep);
    poolp->membasep = NULL;
  }
  poolp->objsize = 0;
  poolp->blksize = 0;
  poolp->numblk = 0;
  poolp->totalsize = 0;
  poolp->memfreedp = NULL;
  poolp->memusedp = NULL;
  poolp->poolinited = FALSE;
  pthread_mutex_unlock(&poolp->mutex);

  /* Uninitialized mutex */
  pthread_mutex_destroy(&poolp->mutex);
}

/*
* NAME :        mempool_alloc
*
* DESCRIPTION : Gets a new block from pool
*
* INPUTS :      poolp - pointer to pool control block
*
* OUTPUTS :     Address of new block
*
* NOTES :       None
*/
void * mempool_alloc(
  mempool_t *poolp)
{
  struct mmblockhead_s *cur_blkp = NULL;
  void *res = NULL;

  if (!poolp)
  {
    printf("%s - Error: Invalid pool.\n", __func__);
    return NULL;
  }

  /* If pool initialized, get a block from memfreed */
  pthread_mutex_lock(&poolp->mutex);
  if (poolp->poolinited)
  {

    /* Point to the first block in free list */
    cur_blkp = poolp->memfreedp;

    /* If there is a free block , move the block to used list.
     * Set linked list parameters for memusedp. The pool will be placed at the
     * head to the used list and memusedp will be pointing to the block.
     */
    if (cur_blkp)
    {
      poolp->memfreedp = poolp->memfreedp->nextp;

      if (poolp->memfreedp)
      {
        poolp->memfreedp->prevp = NULL;
      }

      cur_blkp->prevp = NULL;
      cur_blkp->nextp = poolp->memusedp;
      cur_blkp->used = TRUE;
      poolp->memusedp = cur_blkp;

      /* Pass the address of block.
       * Available memory region starts after the block's header.
       */
      res = (void *) ((void *) cur_blkp + sizeof(struct mmblockhead_s));
    }
    else
    {
      printf("%s - Error: No memory available to allocate.\n", __func__);
    }
  }
  pthread_mutex_unlock(&poolp->mutex);
  
  return res;
}

/*
* NAME :        mempool_is_mem_valid
*
* DESCRIPTION : Verifies if memory belongs to pool and is a valid block
*
* INPUTS :      poolp - pointer to pool control block
*               memp - memory to check
*
* OUTPUTS :     TRUE - If memory is valid
*               FALSE - If memory is not valid
*
* NOTES :       None
*/
boolean mempool_is_mem_valid(
  mempool_t *poolp,
  void *memp)
{
  struct mmblockhead_s *blkp = NULL;

  if (!poolp || !memp)
  {
    printf("%s - Error: Invalid input parameters.\n", __func__);
    return FALSE;
  }

  /* Point to the block's header */
  blkp = (struct mmblockhead_s *)(memp - sizeof(struct mmblockhead_s));

  /* Is memory in the pool memory region? */
  if ((uint8_t *) blkp >= poolp->membasep &&
      (uint8_t *) blkp < (poolp->membasep + poolp->totalsize))
  {

    /* check if the block's address is valid */
    if ((((uint8_t *) blkp - poolp->membasep) % poolp->blksize) == 0 &&
        (((uint8_t *) blkp - poolp->membasep) / poolp->blksize >= 0 ||
        ((uint8_t *) blkp - poolp->membasep) / poolp->blksize < poolp->numblk))
    {
      return TRUE;
    }
  }

  return FALSE;

}

/*
* NAME :        mempool_rel
*
* DESCRIPTION : Release memory pool
*
* INPUTS :      poolp - pointer to pool control block
*               memp - memory to release
*
* OUTPUTS :     TRUE - Success
*               FALSE - Failed
*
* NOTES :       None
*/
boolean mempool_rel(
  mempool_t *poolp,
  void *memp)
{
  struct mmblockhead_s *cur_blkp = NULL;
  boolean res = FALSE;

  if (!poolp || !memp)
  {
    printf("%s - Error: Invalid input parameters.\n", __func__);
    return FALSE;
  }

  /* Check if pool is initialized */
  if (!poolp->poolinited)
  {
    printf("%s - Error: Pool is not initialized.\n", __func__);
  }

  /* Find the block's header */
  cur_blkp = (struct mmblockhead_s *)(memp - sizeof(struct mmblockhead_s));

  /* Is block memory address valid? */
  if (FALSE == mempool_is_mem_valid(poolp, memp))
  {
    /* Here for simplicity, we just return. We could also free the memory
    * by calling free(memp) before returning.
    * But I would prefer to crash or throw an exception since it is
    * a programing or fatal error and a memory out of this pool should have not 
    * been requested to be freed.
    */
    printf("%s - Error: Memory is not pool.\n", __func__);
    return FALSE;
  }


  pthread_mutex_lock(&poolp->mutex);
  do
  {
    if (NULL == cur_blkp ||
        FALSE == cur_blkp->used)
    {
      break;
    }

    /* Set next and previous accordingly */
    if (NULL != cur_blkp->prevp)
    {
      /* This is a node in the middle */
      cur_blkp->prevp->nextp = cur_blkp->nextp;
      cur_blkp->nextp->prevp = cur_blkp->prevp;
    }
    else
    {
      /* This the head node */
      poolp->memusedp = cur_blkp->nextp;
      if (NULL != poolp->memusedp)
      {
        poolp->memusedp->prevp = NULL;
      }
    }

    /* Add the block to the freed list */
    cur_blkp->prevp = NULL;
    cur_blkp->nextp = poolp->memfreedp;
    cur_blkp->used = FALSE;
    if ( NULL != poolp->memfreedp)
    {
      poolp->memfreedp->prevp = cur_blkp;
    }
    poolp->memfreedp = cur_blkp;

    /* Released the memory block successfully */
    res = TRUE;

  } while(0);
  pthread_mutex_unlock(&poolp->mutex);

  return res;
}

/*
* NAME :        mempool_print_stat
*
* DESCRIPTION : Print mempool status for debugging purpose
*
* INPUTS :      poolp - pointer to pool control block
*
* OUTPUTS :     None
*
* NOTES :       This function is not thread safe.
*/
void mempool_print_stat(
  mempool_t *poolp)
{
  printf("pool status: size:%d, numblocks:%d, blocksize:%d, msgsize:%d, mem:%p, memused:%p, memfreed:%p\n",
        poolp->totalsize, poolp->numblk, poolp->blksize, poolp->objsize, poolp->membasep, poolp->memusedp, poolp->memfreedp);
}
