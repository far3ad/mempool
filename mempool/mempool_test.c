#include <stdio.h>
#include <assert.h>

#include "mempool.h"

typedef struct 
{
  uint8_t len;
  uint8_t data[255];
} message_t;


int main(int argc, char **argv)
{
  mempool_t tpool = {0};
  mempool_t *dummy_tpool = NULL;
  int num_msg = 10;
  message_t *msg[num_msg];
  int i = 0;
  void *prev_memaddressp = NULL;
  void *dummy_memaddressp = NULL;

  printf("Testing the pool...:\n");
  /* Testing wrong arguments */
  assert(FALSE == mempool_init(dummy_tpool, num_msg, sizeof(message_t)));
  assert(FALSE == mempool_init(&tpool, 0, sizeof(message_t)));
  assert(FALSE == mempool_init(&tpool, 1, 0));

  /* Verify if mempool_init can create a pool successfully */
  mempool_print_stat(&tpool);
    assert(TRUE == mempool_init(&tpool, num_msg, sizeof(message_t)));
  mempool_print_stat(&tpool);
  printf("mempool_init PASSED\n");

  printf("Testing mempool_rel with unused memory block");
  dummy_memaddressp = (void *)tpool.membasep + tpool.blksize;
  assert(FALSE == mempool_rel(&tpool, dummy_memaddressp));
  printf("... PASSED\n");

  printf("Testing mempool_rel with unordered memory block");
  prev_memaddressp = (message_t *) mempool_alloc(&tpool);
  dummy_memaddressp = (message_t *) mempool_alloc(&tpool);
  assert(TRUE == mempool_rel(&tpool, prev_memaddressp));
  assert(TRUE == mempool_rel(&tpool, dummy_memaddressp));
  printf("... PASSED\n");

  printf("Testing mempool_is_mem_valid ");
  dummy_memaddressp = (message_t *) mempool_alloc(&tpool);
  assert(TRUE == mempool_is_mem_valid(&tpool, dummy_memaddressp));
  /* Increase memory address to point to an incorrect location */
  dummy_memaddressp ++;
  assert(FALSE == mempool_is_mem_valid(&tpool, dummy_memaddressp));
  assert(FALSE == mempool_rel(&tpool, dummy_memaddressp));
  dummy_memaddressp --;
  assert(TRUE == mempool_rel(&tpool, dummy_memaddressp));
  printf("... PASSED\n");

  /* Stress test the pool by using all memory and releasing all at once */
  printf("Testing mempool_alloc, allocate all available memory blocks  ");
  do
  {
    prev_memaddressp = tpool.memfreedp;

    msg[i] = (message_t *) mempool_alloc(&tpool);
    if (!msg[i])
    {
      break;
    }

    printf("Get messages num %d - ", i+1);
    mempool_print_stat(&tpool);

    assert(prev_memaddressp == (void *) tpool.memusedp);

    i++;
  }while(TRUE);
  /* number of allocated messages must be equal to number of messages (num_msg) */
  assert(i == num_msg);

  /* The pool should not be able to allocate any more message */
  assert( NULL ==  (message_t *) mempool_alloc(&tpool));
  printf("Testing mempool_alloc, allocate all memory blocks  PASSED\n");

  printf("\nReleasing all messages...\n\n");
  do
  {
    prev_memaddressp = tpool.memusedp;

    i--;
    printf("Releasing message %d ", i+1);
    
    mempool_rel(&tpool, msg[i]);

    assert((void *) tpool.memfreedp == prev_memaddressp);

    mempool_print_stat(&tpool);
  }while(i != 0);
  printf("Releasing all messages...PASSED\n");


  mempool_destroy(&tpool);
  assert((void *) tpool.memfreedp == NULL);
  assert((void *) tpool.memusedp == NULL);
  assert((void *) tpool.membasep == NULL);
  printf("Testing mempool_destroy... PASSED\n");
  mempool_print_stat(&tpool);
}
