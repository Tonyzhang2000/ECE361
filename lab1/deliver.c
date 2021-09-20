#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>


int main(int argc, char const * argv[]) {

    if(argc != 3) {
        printf("Invalid Input, exiting...\n");
        return 0;
    }

    const char * portNum = argv[2];
    const char * destIp = argv[1];

    //Copied from Beej's guide
    struct addrinfo hints;
    struct addrinfo *serverinfo;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    int status = getaddrinfo(destIp, portNum, &hints, &serverinfo);
    
    if(status != 0) {
        printf("Get address information error, exiting...\n");
        return 0;
    } else {
        printf("Get address information successfully!");
    }

    int socketFD = socket(serverinfo->ai_family, serverinfo->ai_socktype, serverinfo->ai_protocol);

    if(socketFD == -1) {
        printf("Error! Could not create socket...\n");
        return 0;
    } else {
        printf("Socket created successfully!\n");
    }

    int portNumber = atoi(argv[2]);


    freeaddrinfo(serverinfo);
    printf("Hello world\n");
    return 0;
}

