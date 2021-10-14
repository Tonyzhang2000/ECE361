#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

#include "packet.h"

#define MAXBUFLEN 100

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

    printf("Server recieving on port %d...\n", portNumber);

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

    //const int bufferSize = 44;
    char buff[MAXBUFLEN] = {0};
    struct sockaddr_storage socketOutput;
    socklen_t lenOutput = sizeof(socketOutput);

    int bytesReceive = recvfrom(socketFD, (void *) buff, MAXBUFLEN, 0, (struct sockaddr *) &socketOutput, &lenOutput);

    //check if recvfrom is successful
    if(bytesReceive == -1){
        printf("recvfrom error...\n");
    }else{
        printf("%d bytes received!\n", bytesReceive);  //wo jue de ying gai xian xie deliver.c
    }

    //check if the client is sending "ftp" or not
    if(strcmp(buff, "ftp") == 0){
        if((sendto(socketFD, "yes", 3, 0, (struct sockaddr *) &socketOutput, lenOutput)) == -1){
            printf("Error! Could not send message to client...\n");
            return 0;
        }else{
            printf("Yes, Please continue ftp process\n");
        }
    }else{
        //sendto client "no"
        if((sendto(socketFD, "no", 2, 0, (struct sockaddr *) &socketOutput, lenOutput)) == -1){
            printf("Error! Could not send message to client...\n");
            return 0;
        }else{
            printf("No, the msg is not ftp\n");
        }
    }

    char recvItem[2000];                                 //老板我发现这个玩意如果declare在while loop里面就会segfault，为啥啊
    int total_packet_num = 1, current_packet_num = 1;

    while(current_packet_num <= total_packet_num) {

        //initialize packet to receive imformation
        struct packet pack;
        memset(pack.filedata, 0, 1000 * sizeof(char));
        memset(recvItem, 0, sizeof(recvfrom));

        //initialize string to receive message in recvfrom(), set the string size to 2000
        struct sockaddr_storage deliverIn;
        socklen_t lenDeliverIn = sizeof(deliverIn);
        recvfrom(socketFD, (void*) recvItem, 2000, 0, (struct sockaddr *) &deliverIn, &lenDeliverIn);

        printf("Message received: %s\n", &recvItem);

        //now that we received the message, we start to deserialize the packet
        deserialize(&pack, recvItem);

        //update total packet number and current packet num
        total_packet_num = pack.total_frag;
        current_packet_num++;

        //After everything is done, send the ACK message tell the deliver to send a new packet
        if(sendto(socketFD, "ACK", 3, 0, (struct sockaddr *) &socketOutput, lenOutput) == -1) {
            printf("Error! Can't send ACK message\n");
            return 0;
        } else {
            printf("ACK message has been sent!\n");
        }
    }

   close(socketFD);

    return 0;
}

