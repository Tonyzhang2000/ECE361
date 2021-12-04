#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>



//define users
struct User {
    char name[20];
    char key[10];
    int socketFD;
    struct Session *s;
    bool active;
    bool in_session;
    //construct linked list
    struct User *next;
};

//define sessions
struct Session {
    char id[20];
    struct User *user;
    struct Session *next;
};


struct Connection {
    int connectionNum;
    int socketnum;
    pthread_t thread;
    struct User *user;
    struct Connection *next;
};

struct Connection connection[5] = {
    {.connectionNum = 0, .socketnum = -1}, 
    {.connectionNum = 1, .socketnum = -1},
    {.connectionNum = 2, .socketnum = -1},
    {.connectionNum = 3, .socketnum = -1},
    {.connectionNum = 4, .socketnum = -1},
};


struct User user[5] = {
    {.name = "user1", .key = "11"},
    {.name = "user2", .key = "22"},
    {.name = "user3", .key = "33"},
    {.name = "user4", .key = "44"},
    {.name = "user5", .key = "55"}
};

struct Session *active_session_list = NULL;

void initialize_user() {
    for(int i = 0;i < 5;i++) {
        user[i].socketFD = -1;
        user[i].s = NULL;
        user[i].active = false;
        user[i].in_session = false;
        user[i].next = NULL;
    }
    return;
}

void printUserInfo() {
    printf("Users information:\n");
    for(int i = 0;i < 5;i++) {
        printf("User name: %s, password: %s", user[i].name, user[i].key);
        if(user[i].active == false) {
            printf(", status: inactive.\n");
        } else {
            printf(", status: active.\n");
        }
    }
}

/***************************For server to handle diff type**********************************/
bool session_exist(struct Session *s_list, char *session_name){
    if(s_list == NULL){
        return false;
    }
    struct Session *curr = s_list;
    while(curr != NULL){
        if(strcmp(curr->id, session_name) == 0){
            return true;
        }
        curr = curr->next;
    }
    return false;
}

bool user_in_session(struct Session *s_list, char *session_name, struct User user_src){
    if(s_list == NULL){
        return false;
    }
    struct Session *curr = s_list;
    while(curr != NULL){
        if(curr->user == NULL){
            continue;
        }
        if(strcmp(curr->id, session_name) == 0){
            struct User *curr_u = curr->user;
            while(curr_u != NULL){
                if(strcmp(curr_u->name, user_src.name) == 0){
                    return true;
                }
                curr_u = curr_u->next;
            }
        }
        curr = curr->next;
    }
    return false;
}



//create a new session for the user if the name is available
//return the newly constructed session
//check if the session has already exist!!!
struct Session* creat_session(struct Session *s_list, char *session_name){
    //printf("in here\n");
    struct Session *new = malloc(sizeof(struct Session));
    strcpy(new->id, session_name);
    
    if(s_list == NULL){
        s_list = malloc(sizeof(struct Session));
        s_list = new;
        new->next = NULL;
        return s_list;
    }
   // printf("in here2\n");
    struct Session *curr = s_list;
    while(curr->next != NULL){
        curr = curr->next;
    }
    curr->next = new;
    new->next = NULL;
    printf("create session!\n");
    return s_list;
}

//insert the user into the session he want to join
//check if the session is exist!!!
//check if the user has already joined the session!!!
struct Session *join_session(struct Session *s_list, char *session_name, struct User user_src){
    struct Session *curr = s_list;
    while(curr != NULL){
        if(strcmp(curr->id, session_name) == 0){
            break;
        }
        curr = curr->next;
    }
    struct User *new_user = malloc(sizeof(struct User));
    new_user->active = user_src.active;
    strcpy(new_user->name, user_src.name);
    //printf("new user name: %s, %s\n", new_user->name, user_src.name);
    strcpy(new_user->key, user_src.key);
    new_user->in_session = user_src.in_session;
    new_user->socketFD = user_src.socketFD;

    struct User *curr_u = curr->user;
    if(curr_u == NULL){
        curr->user = malloc(sizeof(struct User));
        curr->user = new_user;
        new_user->next = NULL;
        return s_list;
    }
    while(curr_u->next != NULL){
        curr_u = curr_u->next;
    }
    curr_u->next = new_user;
    new_user->next = NULL;
    printf("join session2!\n");
    return s_list;
}


//when last user in sesion leaves, delete the session
struct Session *delete_session(struct Session *s_list, char *session_name){
    struct Session *curr = s_list;
    struct Session *prev = NULL;
    while(curr != NULL){
        if(strcmp(curr->id, session_name) == 0){
            if(prev != NULL){
                prev->next = curr->next;
                curr->next = NULL;
                free(curr->user);
                free(curr);
            }else{
                s_list = curr->next;
                curr->next = NULL;
                free(curr->user);
                free(curr);
            }
            break;
        }
        prev = curr;
        curr = curr->next;
    }

    return s_list;
}

//remove the user from the session he was joined
//if there's no user in the session, delete the session
struct Session *leave_session(struct Session *s_list, char *session_name, struct User user_src){
    struct Session *curr = s_list;
    struct User *u = NULL;
    //printf("session name: %s\n", session_name);
    while(curr != NULL){
        //printf("curr session: %s\n", curr->id);
        if(strcmp(curr->id, session_name) == 0){
            struct User *curr_u = curr->user;
            struct User *prev_u = NULL;
            while(curr_u != NULL){
                if(strcmp(curr_u->name, user_src.name) == 0){
                    if(prev_u != NULL){
                        prev_u->next = curr_u->next;
                        curr_u->next = NULL;
                        free(curr_u);
                    }else{
                        curr->user = curr_u->next;
                        curr_u->next = NULL;
                        free(curr_u);
                    }
                    u = curr->user;
                    break;
                }
                prev_u = curr_u;
                curr_u = curr_u->next;
            }
        }
        curr = curr->next;
    }
    if(u == NULL){
        s_list = delete_session(s_list, session_name);
    }
    return s_list;
}

bool in_session(struct Session *s_list, char* user_src){
    if(s_list == NULL){
        return false;
    }
    struct Session *curr = s_list;
    while(curr != NULL){
        if(curr->user == NULL){
            continue;
        }
        struct User *curr_u = curr->user;
        while(curr_u != NULL){
            if(strcmp(user_src, curr_u->name) == 0){
                return true;
            }
            curr_u = curr_u->next;
        }
        curr = curr->next;
    }
    return false;
}