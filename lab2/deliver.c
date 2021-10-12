#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>

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

    //now we know that file exist, want to find some information of the file
    //first find the file length, which will tellls us the number of packets we need

    FILE *file = fopen(file_to_cp, "r"); //"r" means open a exist file to read
    if(file == NULL) {
        printf("Error! Can't open file...");
        return 0;
    } 

    fseek(file, 0, SEEK_END);  //set the pointer to the end of the file
    int numberPackets = ftell(file) / 1000 + 1; 
    fseek(file, 0, SEEK_SET);  //set the pointer to the start of the file
    printf("We need %d packets\n", numberPackets);

    char **packs = malloc(sizeof(int *) * numberPackets); //creat the packets array
    
    //creat packets one by one
    for(int i = 0;i < numberPackets;i++) {
        printf("Preparing packet number %d...", i + 1);
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

    freeaddrinfo(serverinfo);
    close(socketFD);         //prevent any read or write
    return 0;
}

