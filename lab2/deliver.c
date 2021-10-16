#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>

#include "packet.h"

#define MAXBUFLEN 100

int main(int argc, char const * argv[]) {

    if(argc != 3) {
        printf("Invalid Input, exiting...\n");
        return 0;
    }

    const char * portNum = argv[2];
    const char * destIp = argv[1];

    //Copied from Beej's guide
    struct addrinfo hints;
    struct addrinfo *serverinfo;    //will point to result

    memset(&hints, 0, sizeof hints);   //make struct empty
    hints.ai_family = AF_INET;         //IPv4
    hints.ai_socktype = SOCK_DGRAM;    //UDP
    hints.ai_flags = AI_PASSIVE;       //Fill in my ip address

    int status = getaddrinfo(destIp, portNum, &hints, &serverinfo);
    
    if(status != 0) {
        printf("Get address information error, exiting...\n");
        return 0;
    } else {
        printf("Get address information successfully!\n");
    }

    //get file descriptor
    int socketFD = socket(serverinfo->ai_family, serverinfo->ai_socktype, serverinfo->ai_protocol);

    if(socketFD == -1) {
        printf("Error! Could not create socket...\n");
        return 0;
    } else {
        printf("Socket created successfully!\n");
    }

    int portNumber = atoi(argv[2]);

    //get the input from user ftp<file>
    //check if file exist
    //1. exist, send "ftp" to server
    //2. exit 

    char buff[MAXBUFLEN] = {0};
    struct sockaddr_storage socketOutput;
    socklen_t lenOutput = sizeof(socketOutput);
    
    clock_t start, end;
    double time_used;
    char message[256], file_to_cp[256];
    printf("Input a message of the following form\n\t ftp <file name>\n");
    scanf("%s%s", message, file_to_cp);
    if(strcmp(message, "ftp") != 0 ){
        printf("Error! Didn't find ftp entered\n");
        return 0;
    } else if(access(file_to_cp, F_OK) == -1) {
        printf("File doesn't exist!\n");
        return 0;
    }

    int byteSend = sendto(socketFD, "ftp", 3, 0, serverinfo->ai_addr, serverinfo->ai_addrlen);

    start = clock();
            
    //get msg from server
    //1. yes: print "A file transfer can start"
    //2. no: exit 
    int msg_receive_from_server = recvfrom(socketFD, (void *) buff, MAXBUFLEN, 0,  (struct sockaddr *) &socketOutput, &lenOutput);
    end = clock();
    time_used = (double)(end - start)/CLOCKS_PER_SEC;

    printf("RTT is %lf seconds.\n", time_used);

    if(strcmp(buff, "yes") == 0){
        printf("A file transfer can start\n");
    }else{
        printf("error, can't start file transfer\n");
        return 0;
    }

    //now we know that file exist, want to find some information of the file
    //first find the file length, which will tellls us the number of packets we need

    FILE *file = fopen(file_to_cp, "r"); //"r" means open a exist file to read
    if(file == NULL) {
        printf("Error! Can't open file...");
        return 0;
    } 

    fseek(file, 0, SEEK_END);  //set the pointer to the end of the file
    int numData = ftell(file);
    int numberPackets = numData / 1000 + 1; 
    fseek(file, 0, SEEK_SET);  //set the pointer to the start of the file
    printf("We need %d packets\n", numberPackets);


    for(int curNum = 0;curNum < numberPackets;curNum++) {

        //preparing packets, store packet data
        printf("Preparing packet number %d...", curNum + 1);
        struct packet pack;
        memset(pack.filedata, 0, 1000);
        fread((void*) pack.filedata, sizeof(char), 1000, file);
        pack.total_frag = numberPackets;
        pack.frag_no = curNum;
        if(curNum == numberPackets - 1) {
            pack.size = numData % 1000;
            if(pack.size == 0) pack.size = 1000;
        } else pack.size = 1000;
        pack.filename = file_to_cp;
        printf("Finished!\n");
        printf("%d\n", pack.size);
        //我操我写到这儿发现packet没有存在的意义

        //Item that actually sent
        char sentItem[2000]; 

        //store packet content in the item and calculate the length
        int packSize = serialize(&pack, sentItem);
        packSize += pack.size;

        //debug message
        printf("packet length: %d, buffsize: %d\n", packSize, sizeof(sentItem));
        printf("%s\n", sentItem);


        //send packets, size calculation is wrong
        if(sendto(socketFD, sentItem, packSize, 0, serverinfo->ai_addr, serverinfo->ai_addrlen) == -1) {
            printf("Error! Can't send packet number %d...", curNum+1);
            return 0;
        }  else {
            printf("Packet number %d has been sent...\n", curNum+1);
        }

        //After we sent the message, we expect an "ACK" message from the server
        char message[MAXBUFLEN] = {0};
        struct sockaddr_storage serverOut;
        socklen_t lenServerOut = sizeof(serverOut);
        recvfrom(socketFD, (void*) message, MAXBUFLEN, 0, (struct sockaddr *) &serverOut, &lenServerOut);

        //Check if get the "ACK" message
        if(strcmp(message, "ACK") == 0) {
            printf("ACK message received!\n");
        } else {
            printf("Error! No ACK message received. Exiting...");
        }

    }
    
    freeaddrinfo(serverinfo);
    close(socketFD);         //prevent any read or write
    return 0;
}

