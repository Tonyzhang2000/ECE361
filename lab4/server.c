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
    struct message msg_recv;
    struct message msg_sent;
    char message[1000];

    printf("new connection/thread succcessfully created\n");

    bool already_login = false;
    bool to_quit = false;

    while(1){
        //char recv_message[1000];
        //char sent_message[1000];
        bool send2user = false;

        int recvBytes = recv(*sockNum, message, sizeof message, 0);
        if(recvBytes == -1) {
            printf("recv() failed...");
            close(*sockNum);
            exit(1);
        }

        //printf("message receive from client: %s\n", message);
        deserialize(&msg_recv, message);

        //if the user has already logged in, it can:
        //create session, join session, leave session, send message, query for list, 
        if(already_login == true){
            //check if the user wants to quit
            if(msg_recv.type == QUIT){
                to_quit = true;
            }else if(msg_recv.type == NEW_SESS){

            }else if(msg_recv.type == JOIN){


            }else if(msg_recv.type == LEAVE_SESS){


            }else if(msg_recv.type == MESSAGE){
                

            }else if(msg_recv.type == QUERY){

            }


        }else{ // user first time log in, either login or quit
            if(msg_recv.type == QUIT){
                to_quit = true;
                
            }//end quit
            else if(msg_recv.type == LOGIN){
                already_login = true;
                char *user_name = msg_recv.source;
                char *key = msg_recv.data;
                printf("User: %s, password: %s -> wants to log in\n", user_name, key);

                bool valid_user = false;
                bool first_time_lo = false;
                int user_id = -1;
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
            
            char sentItem[2000];
            serialize(&msg_sent, sentItem);
            
            int bytesSent = send(*sockNum, sentItem, strlen(sentItem), 0);
            if(bytesSent == -1){
                printf("send() fails...");
            }
        }

        if(to_quit){
            break;
        }


    } //end while loop when user wants to quit
    
    //the thread need to quit, safely close the connection  
    //delete all the active session to prevent memory leak
    printf("user wants to exit...\n");
    exit(1);

    
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