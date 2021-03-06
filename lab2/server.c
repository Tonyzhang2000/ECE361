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
    }
    // else{
    //     printf("%d bytes received!\n", bytesReceive); 
    // }

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

    char recvItem[2000];                                 //?????????????????????????????????declare???while loop????????????segfault????????????
    int total_packet_num = 1, current_packet_num = 0;
    FILE * file;

    while(current_packet_num < total_packet_num) {
        //char recvItem[2000];                          //??????????????????????????????????????????recvitem?????????filedata??????????????????2000bytes?????????ack msg???send???????????????????????????????????????
        //initialize packet to receive imformation
        struct packet pack;
        memset(pack.filedata, 0, 1000 * sizeof(char));
        memset(recvItem, 0, sizeof(recvfrom)); 
        pack.filename = (char *)malloc(MAXBUFLEN);
        

        //initialize string to receive message in recvfrom(), set the string size to 2000
        while(1) {
            struct sockaddr_storage deliverIn;
            socklen_t lenDeliverIn = sizeof(deliverIn);

            //just wait here until deliver timeout
            recvfrom(socketFD, (void*) recvItem, 2000, 0, (struct sockaddr *) &deliverIn, &lenDeliverIn);
            if(rand() % 100 < 20) {
                //have 1% rate that we drop what we received
                printf("Packet dropped...\n");
                continue;
            }
            //if not dropped, then we canreach here, exit the loop
            break;
        }
        //recvfrom(socketFD, (void*) recvItem, 2000, 0, (struct sockaddr *) &socketOutput, &lenOutput);

        //printf("Message received: %s\n", recvItem);

        //now that we received the message, we start to deserialize the packet
        deserialize(&pack, recvItem);

        //create new file and write into the file
        if(current_packet_num == 0) {
            char new[] = "new_";
            strcat(new, pack.filename);
            file = fopen(new, "w");
            printf("new file name: %s\n", new);
            if(file != NULL) {
                printf("File created\n");
            } else {
                printf("File created failed\n");
            }
        }
        
        fwrite(pack.filedata, sizeof(char), pack.size, file); 
        
        //update total packet number and current packet num
        total_packet_num = pack.total_frag; 
        current_packet_num++;

        //printf("Packet %d recieved (%d bytes of data)\n", current_packet_num, pack.size);

        //After everything is done, send the ACK message tell the deliver to send a new packet
        if(sendto(socketFD, "ACK", 3, 0, (struct sockaddr *) &socketOutput, lenOutput) == -1) {
            printf("Error! Can't send ACK message\n");  
            return 0;
        } else {
           // printf("ACK message has been sent!\n");     
        }
       
        free(pack.filename);
    }

    fclose(file);
 
    close(socketFD);   

    return 0;
}