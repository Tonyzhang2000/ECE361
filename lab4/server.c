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
//int act_user_count = 0;

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
    //char session_join[200];
    struct Session *session_join = NULL;

    while(1){
        bool send2user = false;
        memset(message, 0, sizeof(char)*1000);
        memset(&msg_recv, 0, sizeof(struct message));

        int recvBytes = recv(sockNum, message, sizeof message, 0);
        if(recvBytes == -1) {
            printf("recv() failed...");
            close(sockNum);
            exit(1);
        }
        if(recvBytes == 0){
            to_exit = true;
            printf("hi\n");
            break;
        }
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
                // if(user[user_id].in_session && !session_exist(active_session_list, session_id)){
                //     //printf("get here\n");
                //     msg_sent.type = NS_NAK;
                //     printf("get here11\n");
                //     int cursor = sprintf((char *)(msg_sent.data), "%s", session_id);
                //     strcpy((char *)(msg_sent.data + cursor), " already joined a session before");
                //     printf("msg sent to user: %s", msg_sent.data);
                //    // printf("you have already joined a session, faliled to create new session\n");
                // }else 
                if(session_exist(active_session_list, session_id)){
                    printf("get here2\n");
                    msg_sent.type = NS_NAK;
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
                    printf("User %s: Successfully created session %s\n", user[user_id].name, session_id);
                    //strcpy((char*)session_join, session_id);
                    if(session_join == NULL){
                        session_join = malloc(sizeof(struct Session));
                        strcpy(session_join->id, session_id);
                        session_join->next = NULL;
                    }else{
                        struct Session *ses = session_join;
                        while(ses->next != NULL){
                            ses = ses->next;
                        }
                        ses->next = malloc(sizeof(struct Session));
                        strcpy(ses->next->id, session_id);
                        ses->next->next = NULL;
                    }
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
                else if(user_in_session(active_session_list, session_id, user[user_id]) == true){
                    msg_sent.type = JN_NAK;
                    int cursor = sprintf((char *)(msg_sent.data), "%s", session_id);
                    strcpy((char *)(msg_sent.data + cursor), " has already joined, you don't need to join again");
                    printf("Warning: user %s: Failed to join session %s\n", user[user_id].name, session_id);
                }
                // //user has already joined other session
                // else if(user_in_session(active_session_list, session_id, user[user_id])){
                //     msg_sent.type = JN_NAK;
                //     //int cursor = sprintf((char *)(msg_sent.data), "%s", session_id);
                //     strcpy((char *)(msg_sent.data), "You already joined other session, please leave first and join again");
                //     printf("Warning: user %s: Failed to join session %s\n", user[user_id].name, session_id);
                // }
                //successfully joined the session
                else{
                    msg_sent.type = JN_ACK;
                    sprintf((char *)(msg_sent.data), "%s", session_id);

                    pthread_mutex_lock(&session_mutex);
                    active_session_list = join_session(active_session_list, session_id, user[user_id]);
                    pthread_mutex_unlock(&session_mutex);

                    pthread_mutex_lock(&user_mutex);
                    user[user_id].in_session = true;
                    pthread_mutex_unlock(&user_mutex);

                    //strcpy((char*)session_join, session_id);
                    if(session_join == NULL){
                        session_join = malloc(sizeof(struct Session));
                        strcpy(session_join->id, session_id);
                        session_join->next = NULL;
                    }else{
                        struct Session *ses = session_join;
                        while(ses->next != NULL){
                            ses = ses->next;
                        }
                        ses->next = malloc(sizeof(struct Session));
                        strcpy(ses->next->id, session_id);
                        ses->next->next = NULL;
                    }
                    
                }     
                send2user = true;
                strcpy(msg_sent.source, user[user_id].name);
            }else if(msg_recv.type == LEAVE_SESS){
                printf("User %s wants to leave the session\n", user[user_id].name);
                //msg_sent.type = LEAVE_SESS;
                
                send2user = true;
                

                char session_leave[100]; 
                strcpy(session_leave, msg_recv.data);


                //if the session leave is valid session
                if(session_exist(active_session_list, session_leave) == false){
                    msg_sent.type = LS_NAK;
                    strcpy(msg_sent.data, "Session not exists");
                    strcpy(msg_sent.source, user[user_id].name);
                }
                //if the user is currently in session leave
                else if(user_in_session(active_session_list, session_leave, user[user_id]) == false){
                    msg_sent.type = LS_NAK;
                    strcpy(msg_sent.data, "User doesn't join this session");
                    strcpy(msg_sent.source, user[user_id].name);
                }else{
                    msg_sent.type = LEAVE_SESS;
                    strcpy(msg_sent.data, msg_recv.data);
                    strcpy(msg_sent.source, user[user_id].name);

                    pthread_mutex_lock(&session_mutex);
                    active_session_list = leave_session(active_session_list, session_leave, user[user_id]);
                    pthread_mutex_unlock(&session_mutex);

                    bool in_ses = in_session(active_session_list, user[user_id].name);
                    // printf("in_session: %d\n", in_ses);
                    // printf("user in session bool: %d\n", user[user_id].in_session);
                    pthread_mutex_lock(&user_mutex);
                    if(!in_ses){
                        user[user_id].in_session = false;
                    }
                    pthread_mutex_unlock(&user_mutex);

                    //delete session from session_list
                    struct Session *curr = session_join;
                    struct Session *prev = NULL;
                    while(curr != NULL){
                        if(strcmp(curr->id, session_leave) == 0){
                            if(prev == NULL){
                                session_join = curr->next;
                                curr->next = NULL;
                                free(curr);
                            }else{
                                prev->next = curr->next;
                                curr->next = NULL;
                                free(curr);
                            }
                        }
                        prev = curr;
                        curr = curr->next;
                    }
                }
                
            }else if(msg_recv.type == MESSAGE){
                printf("User %s wants to send message to the session\n", user[user_id].name);
                //msg_sent.type = MESSAGE;
                char *session_sent;
                char *text;

                session_sent = strtok(msg_recv.data, " ");
                text = strtok(NULL, "\0");

                if(session_exist(active_session_list, session_sent) == false){
                    msg_sent.type = MSG_NAK;
                    strcpy(msg_sent.source, user[user_id].name);
                    strcpy(msg_sent.data, "Session is not valid, please put the correct session ID");

                    msg_sent.size = strlen((char*)(msg_sent.data));
                    char sentItem[2000];
                    serialize(&msg_sent, sentItem);
                    printf("sent Item: %s\n", sentItem);

                    int bytesSent = send(sockNum, sentItem, strlen(sentItem), 0);
                    if(bytesSent == -1){
                        printf("send() fails...");
                    }
                }else if(user_in_session(active_session_list, session_sent, user[user_id]) == false){
                    msg_sent.type = MSG_NAK;
                    strcpy(msg_sent.source, user[user_id].name);
                    strcpy(msg_sent.data, "you are not in this session, please join first and send message");

                    msg_sent.size = strlen((char*)(msg_sent.data));
                    char sentItem[2000];
                    serialize(&msg_sent, sentItem);
                    printf("sent Item: %s\n", sentItem);

                    int bytesSent = send(sockNum, sentItem, strlen(sentItem), 0);
                    if(bytesSent == -1){
                        printf("send() fails...");
                    }
                }
                else{
                    msg_sent.type = MESSAGE;   
                    strcpy(msg_sent.data, text);

                    char source[100];
                    strcpy(source, user[user_id].name);
                    sprintf(source, "%s [%s]", source, session_sent);
                    strcpy(msg_sent.source, source);
                    // printf("msg sent source: %s\n", msg_sent.source);
                    // printf("msg sent data: %s\n", msg_sent.data);
                    msg_sent.size = strlen((char*)(msg_sent.data));
                    
                    char sentItem[2000];
                    serialize(&msg_sent, sentItem);
                    printf("sent Item: %s\n", sentItem);
                    
                    //send msg to the user who are in the session
                    struct Session *curr = active_session_list;
                    while(curr != NULL){
                        if(strcmp(curr->id, session_sent) == 0){
                            struct User *curr_u = curr->user;
                            while(curr_u != NULL){
                                int bytesSent = send(curr_u->socketFD, sentItem, strlen(sentItem), 0);
                                if(bytesSent == -1){
                                    printf("send() fails...");
                                }
                                curr_u = curr_u->next;
                            }
                            break;
                        }
                        curr = curr->next;
                    }
                }
                send2user = false;
            }else if(msg_recv.type == QUERY){
                printf("user wants to query\n");
                msg_sent.type = QU_ACK;
                char *uname = user[user_id].name;
                strcpy(msg_sent.source, uname);
                send2user = true;
                
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
                strcat(msg_sent.data, "no session: ");
                for(int i = 0; i < 5; ++i){
                    if(user[i].in_session == false && user[i].active == true){
                        strcat(msg_sent.data, user[i].name);
                        strcat(msg_sent.data, " ");
                    }
                }
                printf("query result to user: %s\n", msg_sent.data);
            } else if(msg_recv.type == PRIVATE) {
                printf("private message received!\n");
                msg_sent.type = MESSAGE;
                strcpy(msg_sent.data, msg_recv.data);
                strcpy(msg_sent.source, user[user_id].name);
                msg_sent.size = strlen((char*)(msg_sent.data));
                char sentItem[2000];
                serialize(&msg_sent, sentItem);
                printf("sent Item: %s\n", sentItem);

                for(int i = 0;i < 5;i++) {
                    if(strcmp(msg_recv.source, user[i].name) == 0 && user[i].active == true) {
                        printf("receiver valid.\n");
                        int bytesSent = send(user[i].socketFD, sentItem, strlen(sentItem), 0);
                        if(bytesSent == -1){
                            printf("send() fails...");
                        }
                        break;
                    }
                }
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
    user[user_id].in_session = false;
    user[user_id].active = false;
    user[user_id].socketFD = -1;
    
    //clean user in any session info
    if(already_login){
        pthread_mutex_lock(&session_mutex);
        struct Session *ses = session_join;
        while(ses != NULL){
            active_session_list = leave_session(active_session_list, ses->id, user[user_id]);
            ses = ses->next;
        }
        pthread_mutex_unlock(&session_mutex);
    }
    //free session_join
    struct Session *cur = session_join;
    struct Session *pre = NULL;
    while(cur != NULL){
        if(pre == NULL){
            session_join = cur->next;
            cur->next = NULL;
            free(cur);
        }else{
            pre->next = cur->next;
            cur->next = NULL;
            free(cur);
        }
        pre = cur;
        cur = cur->next;
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
    
    for(int i = 0; i < 5; ++i){
        if(connection[i].socketnum == sockNum){
            connection[i].socketnum = -1;
            break;
        }
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

    //?????????guide??????????????????loop??? ??????????????????

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

    printf("exit successfully!\n");
    return 0;
}