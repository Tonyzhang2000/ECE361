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
void *printMessage(void *arg) {
    int sockNum = *(int *) arg;
    free(arg);
    struct message msg;
    while(1) {
        char message[1000];
        memset(message, 0, sizeof(char)*1000);
        memset(&msg, 0, sizeof(struct message));
        int recvNum = recv(sockNum, message, 999, 0);
        if(recvNum == -1) {
            printf("recv() failed...\n");
            close(sockNum);
            return NULL;
        }
        deserialize(&msg, message);
        if(msg.type == LO_ACK) {
            printf("\033[0;32m");
            printf("Login successfully!\n");
            printf("\033[0m");
        } else if(msg.type == LO_NAK) {
            printf("\033[0;31m");
            printf("Login failed...Reason: %s\n", msg.data);
            printf("\033[0m");
        } else if(msg.type == JN_ACK) {
            //When user is logged in and not in other section
            printf("\033[0;32m");
            printf("Successfully joined session: %s.\n", msg.data);
            printf("\033[0m");
            inSession = true;
        } else if(msg.type == LG_ACK) {
            printf("\033[0;32m");
            printf("Logging out...\n");
            printf("\033[0m");
            break;
        } else if(msg.type == LG_NCK) {
            printf("\033[0;31m");
            printf("Logout failed...please try again...\n");
            printf("\033[0m");
        } else if(msg.type == JN_NAK) {
            //When user is currently in other session
            printf("\033[0;31m");
            printf("Join session failed...Error: %s\n", msg.data);
            printf("\033[0m");
        } else if(msg.type == LEAVE_SESS) {
            printf("\033[0;32m");
            printf("Leave section successfully!\n");
            printf("\033[0m");
            inSession = false;
        } else if(msg.type == NS_ACK) {
            printf("\033[0;32m");
            printf("Session created successfully!\n");
            printf("\033[0m");
            inSession = true;
        } else if(msg.type == NS_NAK) {
            printf("\033[0;31m");
            printf("Creat session failed... Error: %s\n", msg.data);
            printf("\033[0m");
        } else if(msg.type == MESSAGE) {
            printf("\033[0;34m");
            printf("%s: %s\n", msg.source, msg.data);
            printf("\033[0m");
        } else if(msg.type == QU_ACK) {
            printf("\033[0;33m");
            //printf("Users and Session: %s\n", msg.data);
            char *text = strtok(msg.data, "-");
            //printf("text: %s\n", text);
            printf("%s: \n", text);
            while(text != NULL){
                text = strtok(NULL, "-");
                if(text != NULL){
                    printf("%s\n", text);
                }
            }
            printf("\033[0m");
        } else {
            printf("\033[0;31m");
            printf("Unknown message received...\n");
            printf("\033[0m");
        }
    }
<<<<<<< HEAD
=======
   // printf(">>");
>>>>>>> edeacf1f102ff1b1fddf681d3e4a6d0d7dc7bc9e
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
            printf("connect() failed...\n");
			close(socketFD);
			continue;
		}
        break;
    }
            

    if(p == NULL) {
        printf("Connection failed...\n");
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
    memset(&msg, 0, sizeof(struct message));

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
    memset(&msg, 0, sizeof(struct message));
    deserialize(&msg, message);
    //这是不是得确认一下LO_ACK
    //new thread to handle print message
    if(msg.type == LO_ACK){
        //printf("login socketFD: %d, %d\n", socketFD);
        int *arg = malloc(sizeof(int));
        *arg = socketFD;
        printf("\033[0;32m");
        printf("User: %s has sucessfully logged in\n", msg.source);
        printf("\033[0;3m");
        pthread_create(thread, NULL, printMessage, (void*)arg);
    }else{
        printf("\033[0;31m");
        printf("log in unsucessfully...Reason: %s.\n", msg.data);
        printf("\033[0m");
        close(socketFD);
        return -1;
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
        printf("logout failed...\n");
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
        printf("Creat session failed...Reason: send() failed, please try again...\n");
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
        printf("Leave session failed...Reason: send() failed, please try again...\n");
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
        printf("Creat session failed...Reason: send() failed, please try again...\n");
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
        printf("List failed...Reason: send() failed, please try again...\n");
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
        printf("Quit failed...Reason: send() failed, please try again...\n");
    }
    return;
}

//sendMessage
void sendMessage(int socketFD, char *name, char *text) {

    char *str = name;
    //strcat(str, ": ");
    //strcat(str, text);
    struct message msg;

    msg.type = MESSAGE;
    strcpy(msg.source, name);
    strcpy(msg.data, text);
    //printf("sent data: %s\n", msg.data);
    msg.size = strlen(msg.data);

    char sentItem[2000];
    serialize(&msg, sentItem);

    int sentBytes = send(socketFD, sentItem, strlen(sentItem), 0);
    if(sentBytes == -1) {
        printf("Send message failed...Reason: send() failed, please try again...\n");
    }
    return;
}

void private(int socketFD, char *name, char *rec, char *text) {

    struct message msg;
    msg.type = PRIVATE;
    strcpy(msg.source, rec);
    strcpy(msg.data, text);
    msg.size = strlen(msg.data);

    char sentItem[2000];
    serialize(&msg, sentItem);

    int sentBytes = send(socketFD, sentItem, strlen(sentItem), 0);
    if(sentBytes == -1) {
        printf("Send message failed...Reason: send() failed, please try again...\n");
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
<<<<<<< HEAD
        sleep(0.5);
        printf(">>");

=======

        sleep(1);
        printf(">>");
>>>>>>> edeacf1f102ff1b1fddf681d3e4a6d0d7dc7bc9e
        fgets(buff, 1000, stdin);
        //fix /n at the end of the string
        //fix the empty input
        char *ptr = buff;
        while(*ptr == ' ') ptr++;
        if(*ptr == '\n') {
            printf("Empty input...\n");
            continue;
        }
        while(*ptr) {
            if(*ptr == '\n') {
                *ptr = 0;
                break;
            }
            ptr++;
        }

        command = strtok(buff, " ");
        if(command == "") {
            printf("Empty!\n");
            continue;
        }

        if(strcmp(command, "/login") == 0) {

            //login
            if(socketFD != -1) {
                printf("The user has already logged in...\n");
                continue;
            }
            name = strtok(NULL, " ");
            key = strtok(NULL, " ");
            ip = strtok(NULL, " ");
            port = strtok(NULL, " ");
            if(name == NULL) {
                printf("Invalid input, please try again.\n");
                continue;
            }
            socketFD = login(name, key, ip, port, &thread);

        } else if(strcmp(command, "/logout") == 0) {
            if(socketFD == -1) {
                printf("Please login first...\n");
                continue;
            }
            inSession = false;
            //logout
            inSession = false;
            socketFD = logout(socketFD, name);
        } else if(strcmp(command, "/joinsession") == 0) {
            if(socketFD == -1) {
                printf("Please login first...\n");
                continue;
            }
            if(inSession == true) {
                printf("Join session failed...already in session...\n");
                continue;
            }
            char *sessionID;
            sessionID = strtok(NULL, " ");
            //joinsession
            joinSession(socketFD, name, sessionID);
        } else if(strcmp(command, "/leavesession") == 0) {
            if(socketFD == -1) {
                printf("Please login first...\n");
                continue;
            }
            if(inSession == false) {
                printf("You are currently not in any session...\n");
                continue;
            }
            //leavesession
            leaveSession(socketFD, name);
        } else if(strcmp(command, "/createsession") == 0) {
            if(socketFD == -1) {
                printf("Please login first...\n");
                continue;
            }
            if(inSession == true) {
                printf("Creat session failed...already in session...\n");
                continue;
            }
            //creatsession
            char *sessionID;
            sessionID = strtok(NULL, " ");
            creatSession(socketFD, name, sessionID);
        } else if(strcmp(command, "/list") == 0) {
            if(socketFD == -1) {
                printf("Please login first...\n");
                continue;
            }
            //list
            list(socketFD, name);
        } else if(strcmp(command, "/quit") == 0) {
            if(socketFD != -1) {
                printf("Please logout first...\n");
                continue;
            }
            //quit
            //quit(socketFD, name);
            break;
        } else if(strcmp(command, "/private") == 0){
            if(socketFD == -1) {
                printf("Please login first...\n");
                continue;
            }
            //send private message
            char *rec, *text;
            rec = strtok(NULL, " ");
            text = strtok(NULL, "\0");
            private(socketFD, name, rec, text);
        } else {
            if(socketFD == -1) {
                printf("Please login first...\n");
                continue;
            }
            if(inSession == false) {
                printf("You are currently not in any session, please join a session first...\n");
                continue;
            }
            char *text;
            text = strtok(NULL, "\0");
            //printf("text1: %s\n", text);
            if(text != NULL){
                sprintf(command, "%s %s", command, text);
            }
            //strcat(command, "");
            //strcat(command, text);
            //printf("Text: %s\n", command);
            //sendmessage
            sendMessage(socketFD, name, command);
        }
    }
    printf("Quit successfully!\n");
    return 0;
}