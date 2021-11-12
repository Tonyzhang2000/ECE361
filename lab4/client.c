#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "packet.h"

char buff[1000];

//helper functions
//login, check if usaerinfo is correct
//return the socket number
int login(char *name, char *key, char *ip, char *port, pthread_t *thread) {

    //printf("%s, %s, %s, %s\n", name, key, ip, port);
    int socketFD;

    //Copied from Beej's guide
    struct addrinfo hints;
    struct addrinfo *serverinfo, *p;    //will point to result

    memset(&hints, 0, sizeof hints);   //make struct empty
    hints.ai_family =  AF_UNSPEC;         //IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;    //TCP

    int status = getaddrinfo(ip, port, &hints, &serverinfo);
    
    if(status != 0) {
        printf("Get address information error, exiting...\n");
        return -1;
    } else {
        printf("Get address information successfully!\n"); 
    }

    for(p = serverinfo;p != NULL;p = p->ai_next) {
        socketFD = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        
        if(socketFD == -1) {
            printf("Error! Could not create socket...\n");
            return -1;
        } 

        if (connect(socketFD, p->ai_addr, p->ai_addrlen) == -1) {
			close(socketFD);
			continue;
		}
        break;
    }
            
    printf("Connected successfully!\n"); 

    if(p == NULL) {
        printf("Connection failed...");
        return -1;
    }

    void *addr;
    char *ipver;
    char ipstr[INET6_ADDRSTRLEN];

    if(p->ai_family == AF_INET) {
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
        addr = &(ipv4->sin_addr);
        ipver = "IPv4";
    } else {
        struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
        addr = &(ipv6->sin6_addr);
        ipver = "IPv6";
    }

    inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
    printf(" %s: %s\n", ipver, ipstr);

    freeaddrinfo(serverinfo);

    struct message msg;

    msg.type = LOGIN;
    strcpy(msg.source, name);
    strcpy(msg.data, key);
    msg.size = strlen(msg.data);

    char sentItem[2000];
    serialize(&msg, sentItem);

    int sentBytes = send(socketFD, sentItem, strlen(sentItem) + 1, 0);
    if(sentBytes == -1) {
        printf("Send() failed...");
        close(socketFD);
        return -1;
    }

    char message[100] = {0};
    int recvBytes = recv(socketFD, message, sizeof message, 0);
    if(recvBytes == -1) {
        printf("recv() failed...");
        close(socketFD);
        return -1;
    }

    deserialize(&msg, message);

    //pthread_create(*thread, NULL, my_func, (void*)&my_arg);

    return socketFD;

}

int main() {

    char* command;
    pthread_t thread;
    int socketFD = -1;

    while(1) {
        fgets(buff, 999, stdin);

        command = strtok(buff, " ");

        if(strcmp(command, "/login") == 0) {

            //login
            if(socketFD != -1) {
                printf("The user has already logged in...");
                continue;
            }
            char *name, *key, *ip, *port;
            name = strtok(NULL, " ");
            key = strtok(NULL, " ");
            ip = strtok(NULL, " ");
            port = strtok(NULL, " ");
            if(name == NULL) {
                printf("Invalid input, please try again.");
                continue;
            }
            socketFD = login(name, key, ip, port, &thread);

        } else if(strcmp(command, "/logout") == 0) {
            if(socketFD == -1) {
                printf("Please login first...");
                continue;
            }
            //logout
        } else if(strcmp(command, "/joinsession") == 0) {
            if(socketFD == -1) {
                printf("Please login first...");
                continue;
            }
            //joinsession
        } else if(strcmp(command, "/leavesession") == 0) {
            if(socketFD == -1) {
                printf("Please login first...");
                continue;
            }
            //leavesession
        } else if(strcmp(command, "/creatsession") == 0) {
            if(socketFD == -1) {
                printf("Please login first...");
                continue;
            }
            //creatsession
        } else if(strcmp(command, "/joinsession") == 0) {
            if(socketFD == -1) {
                printf("Please login first...");
                continue;
            }
            //joinsession
        } else if(strcmp(command, "/list") == 0) {
            if(socketFD == -1) {
                printf("Please login first...");
                continue;
            }
            //list
        } else if(strcmp(command, "/quit") == 0) {
            if(socketFD != -1) {
                printf("Please logout first...");
                continue;
            }
            //quit
            break;
        }

        //sendmessage

    }
    printf("Quit successfully!");
    return 0;
}