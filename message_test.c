#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

#include "message.h"

#define NUM_TIDS 5



/*
* NAME :        thread_fcn
*
* DESCRIPTION : Uses message library to receive and pass message to the next
*               thread. The next thread is selected by incrementing the
*               thread's self ID by 1.
*               It exits when EXIT message is received. EXIT will be passed to
*               next thread before exiting.
*
* INPUTS :      None
*
* OUTPUTS :     None
*
*/
void * thread_fcn(void *arg)
{
  int cid = *(int *)arg;
  message_t **msg = NULL;
  message_t *newmsg = NULL;

  printf("TH%d - %ld started\n", cid, pthread_self());

  do
  {
    /* Wait for message */
    if ( recv(cid, (message_t *)&msg) != 0)
    {
      break;
    }

    /* Find the next destination */
    cid++;
    if (cid >= NUM_TIDS)
    {
      cid = cid % NUM_TIDS;
    }

    printf("TH%d - received %s\n", cid, (*msg)->data);

    /* Create new message */
    newmsg = new_message();
    if(!newmsg)
    {
      break;
    }

    /* Copy received message to new message */
    memcpy(newmsg, *msg, sizeof(message_t));

    /* To stress the memory pool, we delete the receive message and get a new message */
    delete_message(*msg);

    /* Pass the message to another thread */
    send(cid, newmsg);

    /* If message is EXIT, exit */
    if (0 == strcmp((const char * )newmsg->data, "EXIT"))
    {
      break;
    }

  }while(1);

  pthread_exit(NULL);
}


int main(int argc, char *argv[])
{
  pthread_t tid[NUM_TIDS];
  int args[NUM_TIDS];
  message_t * msg;

  /* Create multiple threads */
  for (int i = 0; i < NUM_TIDS; i++)
  {
    args[i] = i;
    pthread_create(&tid[i], NULL, thread_fcn, &args[i]);
  }

  /* Wait 2 seconds to just make sure all threads are initiated 
  * Just for simplicity, I have used sleep. Otherwise this is probably
  * the worst way for synchronizing threads.
  * A mutex guarded counter could be used to let main function know about number
  * of threads that are ready. Once counter == NUM_TIDS, then the process could
  * start. Each thread would hold the mutex and increment the counter by 1.
  */
  sleep(2);

  /* Sending EXIT message */
  printf("%s - Sending EXIT message to threads.\n", __func__);
  msg = new_message();
  strncpy((char *)(msg->data), "EXIT", 4);
  msg->len = 4;
  /* Send message to thread 1 */
  send(0, msg);

  /* Wait for all threads to join */
  for(int i = 0; i < NUM_TIDS; i++)
  {
    pthread_join(tid[i], NULL);
  }

  return 0;
}