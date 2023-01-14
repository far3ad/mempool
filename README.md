# Introduction
This is a simple implementation memory pool and a message queue to test the pool. The memory pool implementation should be easy to port to 32bits processors.
The messaging queue is developed for POSIX.

## MEMPool
mempool is an efficient memory pool which reuses already allocated memory to reduce the times of system call. It allows O(1) allocation without searching a free-list. To achieve this fast allocation, the pool allocator uses blocks of a predefined size. The approach greatly speeds up performance in systems which work with many objects of predefined shapes.

The library calculates required memory during initialisation and allocates appropriate amount of memory from heap. Then it divides memory into block units per user request. Two doubly linked lists are in charge to do the house keeping of the block units: 1.memfreedp and 2. memusedp.
The doubly linked list is used to assure memory allocation and free function are O(1).
In this implementation, memfreedp list is used to store the address of all free (available to use) block units and memusedp list is used to store the address of all used block units.

mempool_t provides control block structure for the pool and it contains doubly linked lists, lock(mutex) and other pool parameters.

## Message library

Message library is a basic message passing library which utilises an optimised memory pool for messaging service.

## Functions
Functions of the library are briefly described below,
### new_message:
To get a new message from memory pool.
### delete_message:
To return a message to memory pool of the library.
### recv:
Message library requires client to register in order to receive a incoming message. The registration happens when a client calls "recv" function. Calling "recv" function, registers a client and makes it reachable by other clients. The following shows the steps,

1. Client calls "recv" function with it is ID and an allocated buffer to receive an incoming message
2. Client will be registered if it is not already. Client ID will be stored in a table for reference.
3. client waits for an incoming signal. The signalling process has been implemented using semaphore.
4. Once a signal is received, the message can be read from client's control block.

### send:
A message can be sent to a client using "send" function if the destination client is already registered. The following shows the steps,
1- The address of message will be stored in the destination's control block
2 - A signal will be generated by posting to the destination's semaphore

## Source files
Here are source files,

    message-+-- Makefile
            |
            +-- message.h
            |
            +-- message.c
            |
            +-- message_test.c
            |
            `-- mempool -+-- mempool.c
                        |
                        +-- mempool.h
                        |
                        `-- mempool_test.c

## Compilation

Use Makefile file,

```bash
make # Cleans and creates two executable files: message-service-test mempool-test
make clean # To clean workspace
```

## Testing

For this assignment I didn't use any UnitTest framework and used assert function to test function. Only mempool library has been covered by unit test. mempool_test.c provides the unit test for mempool. It covers most of common use cases and edge cases.  

message-service-test is a simple application which uses message library to demonstrate the functionality of the message library. The steps are described below,
1. Thread start by waiting to receive a message
2. Once a message received, the thread will copy the message to a new message and delete the old message.
3. the new message will be sent to next thread. The destination thread ID is selected by incrementing the current thread ID.
4. If the message is EXIT, the thread will exit.

