#include <stdio.h>
#include <string.h>
#include "packet.h"
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

#include "packet.h"
#include "database.h"

//global variable
//struct User *active_user_list = NULL;
struct Session *active_session_list = NULL;
int act_user_count = 0;

pthread_mutex_t session_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t user_mutex = PTHREAD_MUTEX_INITIALIZER;


void *newUser(void *socketFD) {
    int *sockNum = (int*)socketFD;
    // struct message msg_recv;
    // struct message msg_sent;
    //char message[2000];

    printf("new connection/thread succcessfully created\n");

    bool already_login = false;
    bool to_exit = false;

    int user_id = -1;

    while(1){
        //char message[2000];
        struct message msg_recv;
        struct message msg_sent;
        memset(&msg_sent, 0, sizeof(struct message));
        memset(&msg_recv, 0, sizeof(struct message));
        //char recv_message[1000];
        //char sent_message[1000];
        printf("here\n");
        //memset(&msg_recv, 0, sizeof(struct message));
        bool send2user = false;
        
        char message[2000];
        memset(message, 0, sizeof(char)*2000);
        printf("message buffer: %s\n", message);
        int recvBytes = recv(*sockNum, message, sizeof(message), 0);
        printf("here1\n");
        if(recvBytes == -1) {
            printf("recv() failed...");
            close(*sockNum);
            exit(1);
        }
        printf("here2\n");
        printf("message recv from client: %s\n", message);
        if(recvBytes == 0){
            //to_exit = true;
            continue;
        }
        printf("here3\n");
        
        deserialize(&msg_recv, message);
        //printf("message recv from client: %s\n", message);
       // memset(&msg_sent, 0, sizeof(struct message));	

        //if the user has already logged in, it can:
        //create session, join session, leave session, send message, query for list, 
        if(already_login == true){
            //check if the user wants to exit
            if(msg_recv.type == EXIT){
                to_exit = true;
            }else if(msg_recv.type == NEW_SESS){
                //printf("user%s wants to create new session\n", user_name);

                

            }else if(msg_recv.type == JOIN){


            }else if(msg_recv.type == LEAVE_SESS){


            }else if(msg_recv.type == MESSAGE){
                

            }else if(msg_recv.type == QUERY){
                printf("user wants to query the list\n");
                msg_sent.type = QU_ACK;
                char *uname = user[user_id].name;
                strcpy(msg_sent.source, uname);
                //printf("source: %s\n", msg_sent.source);
                send2user = true;
                //int cursor = 0;

                for(int i = 0; i < 5; ++i){
                    if(user[i].active == true){
                        //cursor += sprintf((char *)(msg_sent.data) + cursor, "%s", user[i].name);
                        strcpy(msg_sent.data, user[i].name);
                        //cursor += sprintf((char *)(msg_sent.data) + cursor, "\t%d", user[i].s->id);
                    }
                    //msg_sent.data[cursor++] = '\n';

                }
                printf("query result: %s\n", msg_sent.data);
                //strcpy((char *)(msg_sent.data), "query msg");
            }


        }else{ // user first time log in, either login or quit
            if(msg_recv.type == LOGIN){
                printf("here log in\n");
                already_login = true;
                char* user_name = msg_recv.source;
                char* key = msg_recv.data;
                printf("User: %s, password: %s -> wants to log in\n", user_name, key);

                bool valid_user = false;
                bool first_time_lo = false;
                
                //check if the user is valid
                //check if the user is already logged in
                for(int i = 0; i < 5; ++i){
                    //printf("user%d name: %s, key: %s\n", i, user[i].name, user[i].key);
                    //printf(user[i].name == user_name);
                    if(strcmp(user[i].name, user_name)==0 && strcmp(user[i].key, key)==0){
                        valid_user = true;
                        if(user[i].active == false){
                            first_time_lo = true;
                            user_id = i;
                        }
                        break;
                    }
                }
                strcpy(msg_sent.source, user_name);
                
                //printf("bool value: %d, %d, %d\n", valid_user, first_time_lo, user_id);
                if(valid_user && first_time_lo){
                    printf("The user is valid to log in\n");
                    msg_sent.type = LO_ACK;
                    already_login = true;
                    send2user = true;
                    strcpy(msg_sent.data, "");
                    //prevent diff thread modify same user info
                    pthread_mutex_lock(&user_mutex);
                    user[user_id].active = true;
                    user[user_id].socketFD = *sockNum;
                    pthread_mutex_unlock(&user_mutex);
                }else{
                    printf("The user is invalid to log in\n");
                    send2user = true;
                    msg_sent.type = LO_NAK;
                    if(!valid_user){
                        strcpy((char *)(msg_sent.data), "warning->the password is wrong");
                    }else if(!first_time_lo){
                        strcpy((char *)(msg_sent.data), "warning->this user has already logged in");
                    }else{
                        strcpy((char *)(msg_sent.data), "warning->can't find this user");
                    }
                    to_exit = true;
                }
                
            }//end log in
            else{
                msg_sent.type = LO_NAK;
                send2user = true;
                strcpy((char *)(msg_sent.data), "warning: you haven't logged in\n");
            }
        }

        if(send2user){
            printf("want to send msg\n");
            msg_sent.size = strlen((char*)(msg_sent.data));
            
            //char sentItem[2000] = {0};
            memset(message, 0, sizeof(message));
            serialize(&msg_sent, message);
            printf("send message: %s\n", message);
            int bytesSent = send(*sockNum, message, sizeof(message), 0);
            if(bytesSent == -1){
                printf("send() fails...");
            }
        }
        printf("message buffer1: %s\n", message);
        if(to_exit){
            break;
        }


    } //end while loop when user wants to exit
    
    //exit no need to send back ack
    //the thread need to exit, safely close the connection  
    //delete from active session to prevent memory leak
    printf("user wants to exit...\n");

    
    return NULL;
}

int main(int argc, char const * argv[]) {

    if(argc != 2) {
        printf("Invalid Input, exiting...\n");
        return 0;
    }

    //int sessionNum = 0, userNum = 0;

    struct addrinfo hints;
    struct addrinfo *res;    //will point to result

    memset(&hints, 0, sizeof hints);   //make struct empty
    hints.ai_family =  AF_UNSPEC;         //IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;    //TCP
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, argv[1], &hints, &res);

    int socketFD = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    if(socketFD  == -1) {
        printf("Error! Could not create socket...\n");
        return 0;
    } else {
        printf("Socket created successfully!\n");
    }

    //为什么guide里面有时候有loop， 有时候没有啊

    if(bind(socketFD, res->ai_addr, res->ai_addrlen) == -1) {
        printf("Bind fails...exiting...\n");
        return 0;
    } else {
        printf("Bind successfully!\n");
    }


    printf("Server recieving on port %s...\n", argv[1]);

    freeaddrinfo(res);

    if(listen(socketFD, 5) == -1) {
        printf("Listen failed, exiting...\n");
        return 0;
    }

    initialize_user();
    //do{
        while(1) {
            struct sockaddr_storage their_addr;
            socklen_t addr_size;
            addr_size = sizeof their_addr;
            int newFD = accept(socketFD, (struct sockaddr *)&their_addr, &addr_size);
            if(newFD == -1){
                printf("Fail to establish new connection on socket\n");
                break;
            }

            printf("New connection established on socket %d.\n", newFD);

            //new thread on new socket to receive messages
            //find the first connection that is avaliable
            for(int i = 0; i < 5; i++) {
                if(connection[i].socketnum == -1) {
                    connection[i].socketnum = newFD;
                    pthread_create(&connection[i].thread, NULL, newUser, (void*)&connection[i].socketnum);
                    break;
                }
            }
        }
    //}while(1);

    printUserInfo();

    printf("exit successfully!\n");
    return 0;
}


/*****************************client*******************************/

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
        memset(&msg, 0, sizeof(struct message));
        int recvNum = recv(sockNum, message, 999, 0);
        if(recvNum == -1) {
            printf("recv() failed...");
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
        //printf("login socketFD: %d, %d\n", socketFD);
        int *arg = malloc(sizeof(int));
        *arg = socketFD;
        printf("\033[0;32m");
        printf("User: %s has sucessfully logged in\n", msg.source);
        printf("\033[0;3m");
        pthread_create(thread, NULL, printMessage, (void*)arg);
    }else{
        printf("\033[0;31m");
        printf("log in unsucessfully\n");
        printf("\033[0m");
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
    //strcat(str, ": ");
    //strcat(str, text);
    struct message msg;

    msg.type = MESSAGE;
    strcpy(msg.source, name);
    strcpy(msg.data, text);
    printf("sent data: %s\n", msg.data);
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
        } else if(strcmp(command, "/createsession") == 0) {
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
            printf("text1: %s\n", text);
            sprintf(command, "%s %s", command, text);
            // strcat(command, "");
            // strcat(command, text);
            printf("Text: %s\n", command);
            //sendmessage
            sendMessage(socketFD, name, command);
        }
    }
    printf("Quit successfully!\n");
    return 0;
}



/************************server*******************************/
#include <stdio.h>
#include <string.h>
#include "packet.h"
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <math.h>

#include "packet.h"
#include "database.h"

//global variable
//struct User *active_user_list = NULL;
struct Session *active_session_list = NULL;
int act_user_count = 0;

pthread_mutex_t session_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t user_mutex = PTHREAD_MUTEX_INITIALIZER;


void *newUser(void *arg) {
    int sockNum = *(int *)arg;
    free(arg);
    struct message msg_recv;
    struct message msg_sent;
    char message[1000];

    printf("new connection/thread succcessfully created\n");

    bool already_login = false;
    bool to_exit = false;

    int user_id = -1;
    char session_join[200];

    while(1){
        //char recv_message[1000];
        //char sent_message[1000];
        bool send2user = false;
        memset(message, 0, sizeof(char)*1000);
        memset(&msg_recv, 0, sizeof(struct message));

        int recvBytes = recv(sockNum, message, sizeof message, 0);
        if(recvBytes == -1) {
            printf("recv() failed...");
            close(sockNum);
            exit(1);
        }

        //printf("message receive from client: %s\n", message);
        deserialize(&msg_recv, message);
        memset(&msg_sent, 0, sizeof(struct message));

        //if the user has already logged in, it can:
        //create session, join session, leave session, send message, query for list, 
        if(already_login == true){
            //check if the user wants to quit
            if(msg_recv.type == EXIT){
                to_exit = true;
            }else if(msg_recv.type == NEW_SESS){
                printf("user %s wants to create new session\n", user[user_id].name);
                char *session_id = msg_recv.data;
                //check if user has already joined other session
                if(user[user_id].in_session && !session_exist(active_session_list, session_id)){
                    printf("get here\n");
                    msg_sent.type = NS_NAK;
                    //strcpy(msg_sent.source, user[user_id].name);
                    //send2user = true;
                    //int cursor = sprintf((char *)(msg_sent.data), "%s", session_id);
                    //strcpy((char *)(msg_sent.data), "You have already joined other session");
                    printf("get here11\n");
                    int cursor = sprintf((char *)(msg_sent.data), "%s", session_id);
                    strcpy((char *)(msg_sent.data + cursor), " already joined a session before");
                    printf("msg sent to user: %s", msg_sent.data);
                   // printf("you have already joined a session, faliled to create new session\n");
                }else if(session_exist(active_session_list, session_id)){
                    printf("get here2\n");
                    msg_sent.type = NS_NAK;
                    //strcpy(msg_sent.source, user[user_id].name);
                    //send2user = true;
                    int cursor = sprintf((char *)(msg_sent.data), "%s", session_id);
                    strcpy((char *)(msg_sent.data+cursor), " already exist! ");
                }else{
                    msg_sent.type = NS_ACK;

                    pthread_mutex_lock(&session_mutex);
                    //printf("here1\n");
                    active_session_list = creat_session(active_session_list, session_id);
                    //printf("here2\n");
                    pthread_mutex_unlock(&session_mutex);

                    pthread_mutex_lock(&session_mutex);
                    active_session_list = join_session(active_session_list, session_id, user[user_id]);
                    pthread_mutex_unlock(&session_mutex);

                    pthread_mutex_lock(&user_mutex);
                    user[user_id].in_session = true;
                    pthread_mutex_unlock(&user_mutex);

                    //strcpy(msg_sent.source, user[user_id].name);
                    sprintf((char *)(msg_sent.data), "%s", session_id);
                    //send2user = true;
                    printf("User %s: Successfully created session %s\n", user[user_id].name, session_id);
                    strcpy((char*)session_join, session_id);
                }
                send2user = true;
                strcpy(msg_sent.source, user[user_id].name);
            }else if(msg_recv.type == JOIN){
                char *session_id = msg_recv.data;
                printf("user %s wants to join session %s\n", msg_recv.source, msg_recv.data);
                printf("user id: %s\n", user[user_id].name);
                
                //session doesn't exist
                if(!session_exist(active_session_list, session_id)){
                    msg_sent.type = JN_NAK;
                    int cursor = sprintf((char *)(msg_sent.data), "%s", session_id);
                    strcpy((char *)(msg_sent.data + cursor), " not exist, please double check session name");
                    printf("Warning: user %s: Failed to join session %s\n", user[user_id].name, session_id);
                }
                //user has already joined this session
                else if(user[user_id].in_session == true){
                    msg_sent.type = JN_NAK;
                    int cursor = sprintf((char *)(msg_sent.data), "%s", session_id);
                    strcpy((char *)(msg_sent.data + cursor), " has already joined, you don't need to join again");
                    printf("Warning: user %s: Failed to join session %s\n", user[user_id].name, session_id);
                }
                //user has already joined other session
                else if(user_in_session(active_session_list, session_id, user[user_id])){
                    msg_sent.type = JN_NAK;
                    //int cursor = sprintf((char *)(msg_sent.data), "%s", session_id);
                    strcpy((char *)(msg_sent.data), "You already joined other session, please leave first and join again");
                    printf("Warning: user %s: Failed to join session %s\n", user[user_id].name, session_id);
                }
                //successfully joined the session
                else{
                    msg_sent.type = JN_ACK;
                    sprintf((char *)(msg_sent.data), "%s", session_id);

                    pthread_mutex_lock(&session_mutex);
                    active_session_list = join_session(active_session_list, session_id, user[user_id]);
                    pthread_mutex_unlock(&user_mutex);

                    pthread_mutex_lock(&user_mutex);
                    user[user_id].in_session = true;
                    pthread_mutex_unlock(&user_mutex);

                    strcpy((char*)session_join, session_id);
                    
                }
                send2user = true;
                strcpy(msg_sent.source, user[user_id].name);
            }else if(msg_recv.type == LEAVE_SESS){
                printf("User %s wants to leave the session\n", user[user_id].name);
                msg_sent.type = LEAVE_SESS;
                strcpy(msg_sent.data, "");
                strcpy(msg_sent.source, user[user_id].name);
                send2user = true;

                pthread_mutex_lock(&session_mutex);
                active_session_list = leave_session(active_session_list, session_join, user[user_id]);
                pthread_mutex_unlock(&session_mutex);

                pthread_mutex_lock(&user_mutex);
                user[user_id].in_session = false;
                pthread_mutex_unlock(&user_mutex);

                memset(session_join, 0, sizeof(session_join));
            }else if(msg_recv.type == MESSAGE){
                printf("User %s wants to send message to the session\n", user[user_id].name);
                msg_sent.type = MESSAGE;
                strcpy(msg_sent.data, msg_recv.data);
                strcpy(msg_sent.source, user[user_id].name);
                msg_sent.size = strlen((char*)(msg_sent.data));
                
                char sentItem[2000];
                serialize(&msg_sent, sentItem);
                printf("sent Item: %s\n", sentItem);
                int bytesSent = send(sockNum, sentItem, strlen(sentItem), 0);
                if(bytesSent == -1){
                    printf("send() fails...");
                }
                send2user = false;
            }else if(msg_recv.type == QUERY){
                printf("user wants to query\n");
                msg_sent.type = QU_ACK;
                char *uname = user[user_id].name;
                strcpy(msg_sent.source, uname);
                //printf("source: %s\n", msg_sent.source);
                send2user = true;
                //int cursor = 0;
                
                // for(int i = 0; i < 5; ++i){
                //     if(user[i].active == true){
                //         //cursor += sprintf((char *)(msg_sent.data) + cursor, "%s", user[i].name);
                //         strcpy(msg_sent.data, user[i].name);
                //         //cursor += sprintf((char *)(msg_sent.data) + cursor, "\t%d", user[i].s->id);
                        
                //     }
                //     //msg_sent.data[cursor++] = '\n';

                // }
                
                //printf("query result sent to user: %s\n", msg_sent.data);
                
                printf("Active session:\n");
                strcpy(msg_sent.data, "ACTIVE SESSION AND USER-");
                struct Session *curr = active_session_list;
                while(curr != NULL){
                    printf("%s: ", curr->id);
                    strcat(msg_sent.data, curr->id);
                    strcat(msg_sent.data, ": ");
                    struct User *curr_u = curr->user;
                    while(curr_u != NULL){
                        //printf("here aasdf\n");
                        printf("%s ", curr_u->name);
                        strcat(msg_sent.data, curr_u->name);
                        strcat(msg_sent.data, " ");
                        curr_u = curr_u->next;
                    }
                    curr = curr->next;
                    strcat(msg_sent.data, "-");
                    printf("\n");
                }
                printf("query result to user: %s\n", msg_sent.data);
            }


        }else{ // user first time log in, either login or quit
            if(msg_recv.type == EXIT){
                to_exit = true;
            }//end quit
            else if(msg_recv.type == LOGIN){
                already_login = true;
                char *user_name = msg_recv.source;
                char *key = msg_recv.data;
                printf("User: %s, password: %s -> wants to log in\n", user_name, key);

                bool valid_user = false;
                bool first_time_lo = false;
                //check if the user is valid
                //check if the user is already logged in
                for(int i = 0; i < 5; ++i){
                    //printf("user%d name: %s, key: %s\n", i, user[i].name, user[i].key);
                    //printf(user[i].name == user_name);
                    if(strcmp(user[i].name, user_name)==0 && strcmp(user[i].key, key)==0){
                        valid_user = true;
                        if(user[i].active == false){
                            first_time_lo = true;
                            user_id = i;
                        }
                        break;
                    }
                }
                strcpy(msg_sent.source, user_name);
                
                //printf("bool value: %d, %d, %d\n", valid_user, first_time_lo, user_id);
                if(valid_user && first_time_lo){
                    printf("The user is valid to log in\n");
                    msg_sent.type = LO_ACK;
                    already_login = true;
                    send2user = true;
                    strcpy(msg_sent.data, "log in successfully");
                    //prevent diff thread modify same user info
                    pthread_mutex_lock(&user_mutex);
                    user[user_id].active = true;
                    user[user_id].socketFD = sockNum;
                    pthread_mutex_unlock(&user_mutex);
                }else{
                    printf("The user is invalid to log in\n");
                    send2user = true;
                    msg_sent.type = LO_NAK;
                    if(!valid_user){
                        strcpy((char *)(msg_sent.data), "warning->the password is wrong");
                    }else if(!first_time_lo){
                        strcpy((char *)(msg_sent.data), "warning->this user has already logged in");
                    }else{
                        strcpy((char *)(msg_sent.data), "warning->can't find this user");
                    }
                }
                
            }//end log in
            else{
                msg_sent.type = LO_NAK;
                send2user = true;
                strcpy((char *)(msg_sent.data), "warning: you haven't logged in\n");
            }
        }

        if(send2user){
            msg_sent.size = strlen((char*)(msg_sent.data));
            //printf("here sent data: %s\n", msg_sent.data);
            char sentItem[2000];
            serialize(&msg_sent, sentItem);
            //printf("sent Item: %s\n", sentItem);
            int bytesSent = send(sockNum, sentItem, strlen(sentItem), 0);
            if(bytesSent == -1){
                printf("send() fails...");
            }
        }

        if(to_exit){
            break;
        }


    } //end while loop when user wants to quit
    

    //the thread need to quit, safely close the connection  
    //delete all the active session to prevent memory leak
    printf("user wants to exit...\n");
    //close(sockNum);
    user[user_id].in_session = false;
    user[user_id].active = false;
    user[user_id].socketFD = -1;
    
    //clean user in any session info
    if(already_login){
        pthread_mutex_lock(&session_mutex);
        active_session_list = leave_session(active_session_list, session_join, user[user_id]);
        pthread_mutex_unlock(&session_mutex);
    }
    msg_sent.type = LG_ACK;
    strcpy(msg_sent.data, "");
    strcpy(msg_sent.source, user[user_id].name);
    msg_sent.size = strlen((msg_sent.data));
    char sentItem[2000];
    serialize(&msg_sent, sentItem);
    int bytesSent = send(sockNum, sentItem, strlen(sentItem), 0);
    if(bytesSent == -1){
        printf("send() fails...");
        return NULL;
    }
    close(sockNum);
    printf("log out successfully\n");
    return NULL;
}

int main(int argc, char const * argv[]) {

    if(argc != 2) {
        printf("Invalid Input, exiting...\n");
        return 0;
    }

    //int sessionNum = 0, userNum = 0;

    struct addrinfo hints;
    struct addrinfo *res;    //will point to result

    memset(&hints, 0, sizeof hints);   //make struct empty
    hints.ai_family =  AF_UNSPEC;         //IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM;    //TCP
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, argv[1], &hints, &res);

    int socketFD = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    if(socketFD  == -1) {
        printf("Error! Could not create socket...\n");
        return 0;
    } else {
        printf("Socket created successfully!\n");
    }

    //为什么guide里面有时候有loop， 有时候没有啊

    if(bind(socketFD, res->ai_addr, res->ai_addrlen) == -1) {
        printf("Bind fails...exiting...\n");
        return 0;
    } else {
        printf("Bind successfully!\n");
    }


    printf("Server recieving on port %s...\n", argv[1]);

    freeaddrinfo(res);

    if(listen(socketFD, 5) == -1) {
        printf("Listen failed, exiting...\n");
        return 0;
    }

    initialize_user();
    //do{
        while(1) {
            struct sockaddr_storage their_addr;
            socklen_t addr_size;
            addr_size = sizeof their_addr;
            int newFD = accept(socketFD, (struct sockaddr *)&their_addr, &addr_size);
            if(newFD == -1){
                printf("Fail to establish new connection on socket\n");
                break;
            }

            printf("New connection established on socket %d.\n", newFD);

            //new thread on new socket to receive messages
            //find the first connection that is avaliable
            for(int i = 0;i < 5;i++) {
                if(connection[i].socketnum == -1) {
                    connection[i].socketnum = newFD;
                    int *arg = malloc(sizeof(int));
                    *arg = newFD;
                    //pthread_create(&connection[i].thread, NULL, newUser, (void*)&connection[i].socketnum);
                    pthread_create(&connection[i].thread, NULL, newUser, (void*)arg);
                    break;
                }
            }
        }
    //}while(1);

    printf("exit successfully!\n");
    return 0;
}