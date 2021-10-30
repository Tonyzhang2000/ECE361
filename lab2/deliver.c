#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

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
    
//    clock_t start, end;
//    clock_t time_used;
   // time_t start, end;
    struct timeval p1, p2;
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
    
    int t1 = gettimeofday(&p1, NULL);
    int byteSend = sendto(socketFD, "ftp", 3, 0, serverinfo->ai_addr, serverinfo->ai_addrlen);

    //start = time();
    //int t1 = gettimeofday(&p1, NULL);
    //time(&start);
    //p1 = gmtime(&start);
            
    //get msg from server
    //1. yes: print "A file transfer can start"
    //2. no: exit 
    int msg_receive_from_server = recvfrom(socketFD, (void *) buff, MAXBUFLEN, 0,  (struct sockaddr *) &socketOutput, &lenOutput);
    //end = time();
    //time(&end);
    //p2 = gmtime(&end);
    int t2 = gettimeofday(&p2, NULL);
    time_used = p2.tv_usec - p1.tv_usec;

    double samp_rtt = time_used;
    double est_rtt = 2 * samp_rtt;
    double alpha = 0.125, beta = 0.25;
    double dev_rtt = samp_rtt;
    double time_out_interval = est_rtt + 4 * dev_rtt;


    printf("RTT is %.4lf usec.\n", time_used); //unit: usec

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
    //printf("number of bytes of send file: %d\n", numData);
    int numberPackets = numData / 1000 + 1; 
    fseek(file, 0, SEEK_SET);  //set the pointer to the start of the file
    printf("We need %d packets\n", numberPackets);


    for(int curNum = 0; curNum < numberPackets; curNum++) {

        //preparing packets, store packet data
        //printf("Preparing packet number %d...\n", curNum + 1);
        struct packet pack;
        memset(pack.filedata, 0, 1000);
        fread((void*) pack.filedata, sizeof(char), 1000, file);
        //printf("bytes of packet data: %d\n", sizeof(pack.filedata));
        pack.total_frag = numberPackets;
        pack.frag_no = curNum + 1; //frag_no 应该是从1开始
        if(curNum == numberPackets - 1) {
            pack.size = numData % 1000;
            if(pack.size == 0) pack.size = 1000;
        } else pack.size = 1000;
        pack.filename = file_to_cp;
        //printf("Finished, pack size: %d\n", pack.size);
        //我操我写到这儿发现packet没有存在的意义

        //Item that actually sent
        char sentItem[2000]; 

        //store packet content in the item and calculate the length
   
        int packSize = serialize(&pack, sentItem);
        //printf("header file length: %d\n", packSize);
        packSize += pack.size;

        //debug message
        // printf("packet length: %d, buffsize: %d\n", packSize, sizeof(sentItem));
        // printf("sent item:%s\n", sentItem);

        //set timer
        int sent_num = 0;

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = time_out_interval;

        if(setsockopt(socketFD, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0) {
            printf("Error!");
        }


        while(1) {
            if(sent_num > 3) {
                printf("Lost connection...");
                return 0;
            }

            t1 = gettimeofday(&p1, NULL); 
            if(sendto(socketFD, sentItem, packSize, 0, serverinfo->ai_addr, serverinfo->ai_addrlen) == -1) {
                printf("Error! Can't send packet number %d...", curNum+1);
                return 0;
            }  else {
                printf("Packet number %d has been sent (%d bytes of data)\n", curNum+1, pack.size);
            }

            //After we sent the message, we expect an "ACK" message from the server
            char message[MAXBUFLEN] = {0};
            struct sockaddr_storage serverOut;
            socklen_t lenServerOut = sizeof(serverOut);
            if(recvfrom(socketFD, (void*) message, MAXBUFLEN, 0, (struct sockaddr *) &serverOut, &lenServerOut) == -1) {
                //timeout happens, retransmit
                sent_num++;
                printf("Start retransmit number %d...", sent_num);
                continue;
            }

            //if we reach here, then we receive things successfully, update timeout interval
            t2 = gettimeofday(&p2, NULL);
            samp_rtt = p2.tv_usec - p1.tv_usec;
            est_rtt = alpha * samp_rtt + (1 - alpha) * est_rtt;
            dev_rtt = (1 - beta) * dev_rtt + beta * fabs(samp_rtt - est_rtt);
            time_out_interval = est_rtt + 4 * dev_rtt;
            //Check if get the "ACK" message
            if(strcmp(message, "ACK") == 0) {
                //this is what we expect, exit while loop
                printf("ACK message received!\n");
                break;
            } else {
                printf("Error! No ACK message received. Exiting...");
            }
        }
    }
    
    freeaddrinfo(serverinfo);
    close(socketFD);         //prevent any read or write
    return 0;
}