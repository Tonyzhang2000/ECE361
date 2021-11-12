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


void initialize() {
    for(int i = 0;i < 3;i++) {
        user[i].socketFD = -1;
        user[i].s = NULL;
        user[i].active = false;
    }
    return;
}

void printUserInfo() {
    printf("Users information:\n");
    for(int i = 0;i < 3;i++) {
        printf("User name: %s, password: %s", user[i].name, user[i].key);
        if(user[i].active == false) {
            printf(", status: inactive.\n");
        } else {
            printf(", status: active.\n");
        }
    }
}