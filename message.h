#ifndef MESSAGE_H
#define MESSAGE_H
#include <stdint.h>

/*
* NAME :        message_t
*
* DESCRIPTION : Message service data type
*
* MEMBERS :     len - Length of data buffer
*               data[] - Data buffer
*
* NOTES :      None
*/
typedef struct 
{
  uint8_t len;
  uint8_t data[255];
} message_t;


extern message_t * new_message(void);

extern void delete_message(message_t *msg);

extern int send(
  uint8_t destination_id,
  message_t* msg);

extern int recv(
  uint8_t receiver_id,
  message_t* msg);

#endif