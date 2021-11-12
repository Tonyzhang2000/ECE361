#include <stdio.h>
#include <string.h>
#include "packet.h"
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

#include "packet.h"
#include "database.h"

int main(int argc, char const * argv[]) {

    if(argc != 2) {
        printf("Invalid Input, exiting...\n");
        return 0;
    }

    //int sessionNum = 0, userNum = 0;

    struct addrinfo hints;
    struct addrinfo *res;    //will point to result

    memset(&hints, 0, sizeof hints);   //make struct empty
    hints.ai_family =  AF_UNSPEC;         //IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;    //TCP
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, argv[1], &hints, &res);

    int socketFD = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    if(socketFD  == -1) {
        printf("Error! Could not create socket...\n");
        return 0;
    } else {
        printf("Socket created successfully!\n");
    }

    //为什么guide里面有时候有loop， 有时候没有啊

    if(bind(socketFD, res->ai_addr, res->ai_addrlen) == -1) {
        printf("Bind fails...exiting...\n");
        return 0;
    } else {
        printf("Bind successfully!\n");
    }


    printf("Server recieving on port %s...\n", argv[1]);

    freeaddrinfo(res);

    if(listen(socketFD, 5) == -1) {
        printf("Listen failed, exiting...\n");
        return 0;
    }

    while(1) {
        struct sockaddr_storage their_addr;
        socklen_t addr_size;
        addr_size = sizeof their_addr;
        int newFD = accept(socketFD, (struct sockaddr *)&their_addr, &addr_size);

        printf("New connection established on socket %d.\n", newFD);

        //new thread on new socket to receive messages
        //pthread_create(thread, NULL, printMessage, (void*)&socketFD);

    }

    initialize();
    //printUserInfo();


} 