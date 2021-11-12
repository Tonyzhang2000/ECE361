# CC=gcc
# all: server deliver
# server: server.o
# deliver: deliver.o
# clean:
# 		rm -f *.o server deliver
LDFLAGS=-pthread
all: server client 
server: server.c packet.h database.h
	gcc server.c -o server
deliver: client.c packet.h 
	gcc clinet.c -o client
clean:  
	rm -f ${server client}