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
bool inSession;

//helper functions

//printMessage
//Use multi-thread to print the reveived message
void *printMessage(void *socketFD) {
    int *sockNum = (int*)socketFD;
    struct message msg;
    while(1) {
        char message[1000];
        int recvNum = recv(*sockNum, message, sizeof message, 0);
        if(recvNum == -1) {
            printf("recv() failed...");
            close(*sockNum);
            return NULL;
        }
        deserialize(&msg, message);
        if(msg.type == LO_ACK) {
            printf("Login successfully!\n");
        } else if(msg.type == LO_NAK) {
            printf("Login failed...Reason: %s\n", msg.data);
        } else if(msg.type == JN_ACK) {
            //When user is logged in and not in other section
            printf("Successfully joined session: %s.\n", msg.data);
            inSession = true;
        } else if(msg.type == LG_ACK) {
            printf("Logging out...");
            break;
        } else if(msg.type == LG_NCK) {
            printf("Logout failed...please try again...");
        } else if(msg.type == JN_NAK) {
            //When user is currently in other session
            printf("Join session failed...Error: %s.\n", msg.data);
        } else if(msg.type == LEAVE_SESS) {
            printf("Leave section successfully!\n");
            inSession = false;
        } else if(msg.type == NS_ACK) {
            printf("Session created successfully!\n");
        } else if(msg.type == NS_NAK) {
            printf("Creat session failed... Error: %s", msg.data);
        } else if(msg.type == MESSAGE) {
            printf("%s: %s\n", msg.source, msg.data);
        } else if(msg.type == QU_ACK) {
            printf("Users and Session: %s.\n", msg.data);
        } else {
            printf("Unknown message received...");
        }
    }

    return NULL;
}


//add close socketFD everytime, or it will cause seg fault on the server side.
//login, check if userinfo is correct
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
            printf("connect() failed...");
			close(socketFD);
			continue;
		}
        break;
    }
            

    if(p == NULL) {
        printf("Connection failed...");
        close(socketFD);
        return -1;
    } else {
        printf("Connected successfully!\n"); 
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

    int sentBytes = send(socketFD, sentItem, strlen(sentItem), 0);
    if(sentBytes == -1) {
        printf("Send() failed...\n");
        close(socketFD);
        return -1;
    }

    char message[100] = {0};
    int recvBytes = recv(socketFD, message, sizeof message, 0);
    if(recvBytes == -1) {
        printf("recv() failed...\n");
        close(socketFD);
        return -1;
    }

    //printf("message received from server: %s\n", message);
    deserialize(&msg, message);
    //这是不是得确认一下LO_ACK
    //new thread to handle print message
    if(msg.type == LO_ACK){
        printf("User: %s has sucessfully logged in\n", msg.source);
        pthread_create(thread, NULL, printMessage, (void*)&socketFD);
    }else{
        printf("log in unsucessfully\n");
        close(socketFD);
    }

    return socketFD;

}

//logout
//if success, return -1, or return the current socket number
int logout(int socketFD, char *name) {

    struct message msg;

    msg.type = EXIT;
    strcpy(msg.source, name);
    strcpy(msg.data, "");
    msg.size = 0;

    char sentItem[2000];
    serialize(&msg, sentItem);

    int sentBytes = send(socketFD, sentItem, strlen(sentItem), 0);
    if(sentBytes == -1) {
        printf("logout failed...");
        return socketFD;
    }
    close(socketFD);
    return -1;
}

//joinsession
void joinSession(int socketFD, char *name, char *sessionID) {
    struct message msg;

    msg.type = JOIN;
    strcpy(msg.source, name);
    strcpy(msg.data, sessionID);
    msg.size = strlen(msg.data);

    char sentItem[2000];
    serialize(&msg, sentItem);

    int sentBytes = send(socketFD, sentItem, strlen(sentItem), 0);
    if(sentBytes == -1) {
        printf("Creat session failed...Reason: send() failed, please try again...");
    }
    return;
}

//leaveSession
//是不是可以declare一个全局的bool, 看是不是已经in section
//这样的话join也能稍微简单一点
void leaveSession(int socketFD, char *name) {

    struct message msg;

    msg.type = LEAVE_SESS;
    strcpy(msg.source, name);
    strcpy(msg.data, "");
    msg.size = 0;

    char sentItem[2000];
    serialize(&msg, sentItem);

    int sentBytes = send(socketFD, sentItem, strlen(sentItem), 0);
    if(sentBytes == -1) {
        printf("Leave session failed...Reason: send() failed, please try again...");
    }
    return;
}

//creatSession
void creatSession(int socketFD, char *name, char *sessionID) {

    struct message msg;

    msg.type = NEW_SESS;
    strcpy(msg.source, name);
    strcpy(msg.data, sessionID);
    msg.size = strlen(sessionID);

    char sentItem[2000];
    serialize(&msg, sentItem);

    int sentBytes = send(socketFD, sentItem, strlen(sentItem), 0);
    if(sentBytes == -1) {
        printf("Creat session failed...Reason: send() failed, please try again...");
    }
    return;
}

//List
void list(int socketFD, char *name) {
    struct message msg;

    msg.type = QUERY;
    strcpy(msg.source, name);
    strcpy(msg.data, "");
    msg.size = 0;

    char sentItem[2000];
    serialize(&msg, sentItem);

    int sentBytes = send(socketFD, sentItem, strlen(sentItem), 0);
    if(sentBytes == -1) {
        printf("List failed...Reason: send() failed, please try again...");
    }
    return;
}

//quit
void quit(int socketFD, char *name) {

    struct message msg;

    msg.type = QUIT;
    strcpy(msg.source, name);
    strcpy(msg.data, "");
    msg.size = 0;

    char sentItem[2000];
    serialize(&msg, sentItem);

    int sentBytes = send(socketFD, sentItem, strlen(sentItem), 0);
    if(sentBytes == -1) {
        printf("Quit failed...Reason: send() failed, please try again...");
    }
    return;
}

//sendMessage
void sendMessage(int socketFD, char *name, char *text) {

    char *str = name;
    strcat(str, ": ");
    strcat(str, text);
    struct message msg;

    msg.type = MESSAGE;
    strcpy(msg.source, name);
    strcpy(msg.data, str);
    msg.size = strlen(msg.data);

    char sentItem[2000];
    serialize(&msg, sentItem);

    int sentBytes = send(socketFD, sentItem, strlen(sentItem), 0);
    if(sentBytes == -1) {
        printf("Send message failed...Reason: send() failed, please try again...");
    }
    return;
}


int main() {

    char* command;
    char *name, *key, *ip, *port;
    pthread_t thread;
    int socketFD = -1;
    inSession = false;

    while(1) { 

        memset(buff, 0, sizeof buff);
        fgets(buff, 999, stdin);

        //fix /n at the end of the string
        char *ptr = buff;
        while(*ptr) {
            if(*ptr == '\n') {
                *ptr = 0;
                break;
            }
            ptr++;
        }

        command = strtok(buff, " ");

        if(strcmp(command, "/login") == 0) {

            //login
            if(socketFD != -1) {
                printf("The user has already logged in...");
                continue;
            }
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
            socketFD = logout(socketFD, name);
        } else if(strcmp(command, "/joinsession") == 0) {
            if(socketFD == -1) {
                printf("Please login first...");
                continue;
            }
            if(inSession == true) {
                printf("Join session failed...already in session...");
                continue;
            }
            char *sessionID;
            sessionID = strtok(NULL, " ");
            //joinsession
            joinSession(socketFD, name, sessionID);
        } else if(strcmp(command, "/leavesession") == 0) {
            if(socketFD == -1) {
                printf("Please login first...");
                continue;
            }
            if(inSession == false) {
                printf("You are currently not in any session...");
                continue;
            }
            //leavesession
            leaveSession(socketFD, name);
        } else if(strcmp(command, "/creatsession") == 0) {
            if(socketFD == -1) {
                printf("Please login first...");
                continue;
            }
            if(inSession == true) {
                printf("Creat session failed...already in session...");
                continue;
            }
            //creatsession
            char *sessionID;
            sessionID = strtok(NULL, " ");
            creatSession(socketFD, name, sessionID);
        } else if(strcmp(command, "/list") == 0) {
            if(socketFD == -1) {
                printf("Please login first...");
                continue;
            }
            //list
            list(socketFD, name);
        } else if(strcmp(command, "/quit") == 0) {
            if(socketFD != -1) {
                printf("Please logout first...");
                continue;
            }
            //quit
            quit(socketFD, name);
            break;
        } else {
            if(socketFD == -1) {
                printf("Please login first...");
                continue;
            }
            if(inSession == false) {
                printf("You are currently not in any session, please join a session first...");
                continue;
            }
            char *text;
            text = strtok(NULL, "\0");
            strcat(command, text);
            printf("Text: %s\n", command);
            //sendmessage
            sendMessage(socketFD, name, command);
        }
    }
    printf("Quit successfully!\n");
    return 0;
}