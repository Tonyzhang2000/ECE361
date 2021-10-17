# CC=gcc
# all: server deliver
# server: server.o
# deliver: deliver.o
# clean:
# 		rm -f *.o server deliver

all: server deliver 
server: server.c packet.h
	gcc server.c -o server
deliver: deliver.c packet.h
	gcc deliver.c -o deliver
clean:  
	rm -f ${server deliver}