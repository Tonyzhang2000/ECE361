#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main(int argc, char const * argv[]) {

    if(argc != 2) {
        printf("Invalid Input, exiting...\n");
        return 0;
    }

    int portNumber = atoi(argv[1]);
    int socketFD = socket(AF_INET, SOCK_DGRAM, 0);

    if(socketFD  == -1) {
        printf("Error! Could not create socket...\n");
        return 0;
    } else {
        printf("Socket created successfully!\n");
    }

    printf("Opening port number %d...\n", portNumber);

    struct sockaddr_in socketInput;
    socketInput.sin_family = AF_INET;
    socketInput.sin_addr.s_addr = htonl(INADDR_ANY);    //point to all local address
    socketInput.sin_port = htons(portNumber);           //to the port thet I want to point to

    int bindnum = bind(socketFD, (struct sockaddr *) &socketInput, (socklen_t) sizeof(socketInput));

    if(bindnum == -1) {
        printf("Bind fails, exiting...\n");
        return 0;
    } else {
        printf("Bind successfully!\n");
    }

    int bufferSize = 44;
    int buff[44] = {0};
    struct sockaddr_in socketOutput;
    socklen_t lenOutput;

    int bytesReceive = recvfrom(socketFD, (void *) buff, bufferSize, 0, (struct sockaddr *) &socketOutput, &lenOutput);

    printf("%d bytes received!", bytesReceive);  //wo jue de ying gai xian xie deliver.c

    return 0;
}

