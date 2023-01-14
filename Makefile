GCCFLAGS = -Wall \
					  -fdiagnostics-color=always \
						-Werror=return-type \
						-g


LIBS = -lpthread

all: clean message-service-test mempool-test

message-service-test: message_test.o message.o mempool.o
		gcc $(GCCFLAGS) -o  message-service-test message_test.o message.o mempool.o $(LIBS)

mempool-test: mempool_test.o mempool.o
		gcc $(GCCFLAGS) -o  mempool-test mempool_test.o mempool.o $(LIBS)

message_test.o: message_test.c
		gcc  $(LIBS) $(GCCFLAGS) -c message_test.c

message.o: message.c message.h
		gcc $(LIBS) $(GCCFLAGS) -c ./message.c ./message.h

mempool.o: ./mempool/mempool.c ./mempool/mempool.h
		gcc $(LIBS) $(GCCFLAGS) -c ./mempool/mempool.c

mempool_test.o: ./mempool/mempool_test.c
		gcc  $(LIBS) $(GCCFLAGS) -c ./mempool/mempool_test.c

clean:
		rm -f *.o *.gch message-service-test mempool-test