
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#include "message.h"
#include "mempool/mempool.h"


#define MAX_NUM_MSG 20
#define MAX_CLIENT_255 255

#define SUCCESS 0
#define ERROR -1


/*
* NAME :        client_ctrl_s
*
* DESCRIPTION : Client control block
*
* MEMBERS :     valid - To set when control block is initialized and registered
*               tid - Thead IDs
*               sem - Semaphore for signaling
*               datap - Pointer to user data
*
* NOTES :      None
*/
struct client_ctrl_s
{
  boolean valid;
  pthread_t tid;
  sem_t sema;
  void *datap;
};

/* Since uint8_t data type for client_id is used, max number of client is 255
* For simplicity a static table is used to store clients information.
* This can be improved bu using hash table for cases where number of clients are
* significantly large.
*/
static struct client_ctrl_s cidtable[255] = {0};

/* Memory pool control block */
static mempool_t _message_pool = {0};

/*
* NAME :        client_find
*
* DESCRIPTION :
*
* INPUTS :      client_id - ID of client in message context
*
* OUTPUTS :     Pointer to client control block
*
* NOTES :       It is a static API
*/
static struct client_ctrl_s * client_find(
  uint8_t client_id)
{
  if (client_id <= MAX_CLIENT_255 &&
      cidtable[client_id].valid == TRUE)
  {
    return &cidtable[client_id];
  }

  return NULL;
}

/*
* NAME :        thread_find_cid
*
* DESCRIPTION : Returns Client ID from thread ID
*
* INPUTS :      client_id - ID of client in message service context
*
* OUTPUTS :     Client Id
*
* NOTES :       It is a static API
*/
static int thread_find_cid(
  pthread_t id)
{
  for(uint8_t i = 0; i < MAX_CLIENT_255; i++)
  {
    if (cidtable[i].tid == id && cidtable[id].valid == TRUE)
    {
      return i;
    }
  }

  return -1;
}

/*
* NAME :        signal_reg
*
* DESCRIPTION : To register to receive signal
*
* INPUTS :      client_id - ID of message service's client
*               thread_id - Thread ID of message service's client
*
* OUTPUTS :     SUCCESS - Success
*               ERROR - Failed
*
* NOTES :       It is a static API
*/
static int signal_reg(
  uint8_t client_id,
  pthread_t thread_id)
{
  if (!cidtable[client_id].valid || thread_id != cidtable[client_id].tid)
  {
    /* add the thread to the table of clients */
    if (sem_init (&cidtable[client_id].sema, 0, 0) == SUCCESS)
    {
      cidtable[client_id].tid = thread_id;
      cidtable[client_id].valid = TRUE;
      return SUCCESS;
    }
    else
    {
      printf("%s - Error: Cannot initialize semaphore\n", __func__);
      return ERROR;
    }
  }

  return ERROR;
}

/*
* NAME :        signal_send
*
* DESCRIPTION : To send a signal to a client
*
* INPUTS :      client - client control block
*
* OUTPUTS :     SUCCESS - Success
*               ERROR - Failed
*
* NOTES :       It is a static API
*/
static int signal_send(
  struct client_ctrl_s *client)
{
  if (client)
  {
    return sem_post(&client->sema);
  }

  return ERROR;
}

/*
* NAME :        signal_wait
*
* DESCRIPTION : Wait to receive a signal
*
* INPUTS :      client - client control block
*
* OUTPUTS :     ERROR - failure
*               SUCCESS - Successful
*
* NOTES :       It is a static API.
*               It is a blocking API and uses Semaphore which should have
*               minimum overhead.
*/
static int signal_wait(
  struct client_ctrl_s *client
)
{
  if (!client ||
      !client->valid)
  {
    return ERROR;
  }

  if (sem_wait(&client->sema) != SUCCESS)
  {
    printf("%s - Error: Cannot post semaphore\n", __func__);
    return ERROR;
  }

  return SUCCESS;
}

/*
* NAME :        new_message
*
* DESCRIPTION : Get a new message
*
* INPUTS :      None
*
* OUTPUTS :     Returns a new message type message_t
*
* NOTES :       It uses mempool library to allocate memory
*/
message_t * new_message(
  void)
{
  if (!_message_pool.poolinited &&
      !mempool_init(&_message_pool, MAX_NUM_MSG, sizeof(message_t)))
  {
    printf("%s - Error: Cannot initialize memory pool.\n", __func__);
    return NULL;
  }

  return (message_t *) mempool_alloc(&_message_pool);
}

/*
* NAME :        delete_message
*
* DESCRIPTION : Deletes a message
*
* INPUTS :      msg - message to delete
*
* OUTPUTS :     None
*
* NOTES :       None
*/
void delete_message(message_t *msg)
{
  mempool_rel(&_message_pool, (void *) msg);
}

/*
* NAME :        send
*
* DESCRIPTION : Sends a message to client by ID
*
* INPUTS :      destination_id - ID of destination client
*               msg - message to send
*
* OUTPUTS :     ERROR - failure
*               SUCCESS - Successful
*
* NOTES :       None
*/
int send(
  uint8_t destination_id,
  message_t* msg)
{
  struct client_ctrl_s *client = NULL;

  if (!msg)
  {
    printf("%s - Error: invalid message.\n", __func__);
    return ERROR;
  }

  client = client_find(destination_id);
  if (!client)
  {
    printf("%s - Error: invalid client.\n", __func__);
    return ERROR;
  }

  /* Store message address in client control block */
  client->datap = (void *) msg;

  /* Send a signal to client */
  return signal_send(client);
}

/*
* NAME :        recv
*
* DESCRIPTION : Waits to receive a message
*
* INPUTS :      msg - message to receive
*
* OUTPUTS :     ERROR - failure
*               SUCCESS - Successful
*
* NOTES :       This is a blocking API.
*/
int recv(
  uint8_t receiver_id,
  message_t* msg)
{
  struct client_ctrl_s *client = NULL;
  pthread_t tid = 0;
  client = client_find(receiver_id);

  if (!client)
  {
    tid = pthread_self();
    if (SUCCESS != signal_reg(receiver_id, tid))
    {
      printf("%s - Error: Cannot register.\n", __func__);
      return ERROR;
    }

    client = client_find(receiver_id);
  }

  if (client && signal_wait(client) == SUCCESS)
  {
    *((void **)msg) = &client->datap;
    return SUCCESS;
  }
  else
  {
    printf("%s - Error: Cannot receive message.\n", __func__);
  }

  return ERROR;
}
