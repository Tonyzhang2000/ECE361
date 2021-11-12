# CC=gcc
# all: server deliver
# server: server.o
# deliver: deliver.o
# clean:
# 		rm -f *.o server deliver
all: server client 
server: server.c packet.h database.h
	gcc server.c -o server -pthread
client: client.c packet.h database.h
	gcc client.c -o client -pthread
clean:  
	rm -f ${server client}