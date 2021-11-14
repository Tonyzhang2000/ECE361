#ifndef PACKET_H
#define PACKET_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define LOGIN 0
#define LO_ACK 1
#define LO_NAK 2
#define EXIT 3
#define JOIN 4
#define JN_ACK 5
#define JN_NAK 6
#define LEAVE_SESS 7
#define NEW_SESS 8
#define NS_ACK 9
#define MESSAGE 10
#define QUERY 11
#define QU_ACK 12
#define LG_ACK 13
#define LG_NCK 14
#define NLEAVE_SESS 15
#define NS_NAK 16


struct message {
    unsigned int type;
    unsigned int size;
    unsigned char source[20];
    unsigned char data[1000];
};

int serialize(const struct message *pack, char *buff) {

    //clear the string
    memset(buff, 0, 2000);

    int headerLen = sprintf(buff, "%d:%d:%s:", pack->type, pack->size, pack->source);

    memcpy(buff + headerLen, pack->data, 1000);

    return headerLen;
}

void deserialize(struct message *pack, const char *buff) {

    char str[100];
    int curidx = 0, len = 0;

    //store packet type into the pack
    for(int i = 0; ;i++) {
        if(buff[i+curidx] == ':') {
            len = i;
            break;
        }
    }
    memset(str, 0, sizeof(str));
    memcpy(str, buff + curidx, len);
    pack->type = atoi(str);
    curidx += len;
    curidx++;

    //store size into the pack
    for(int i = 0; ;i++) {
        if(buff[i+curidx] == ':') {
            len = i;
            break;
        }
    }
    memset(str, 0, sizeof(str));
    memcpy(str, buff + curidx, len);
    pack->size = atoi(str);
    curidx += len;
    curidx++;

    //store source into the pack
    for(int i = 0; ;i++) {
        if(buff[i+curidx] == ':') {
            len = i;
            break;
        }
    }

    memset(str, 0, sizeof(str));
    memcpy(str, buff + curidx, len);
    strcpy((pack->source), str);
    curidx += len; 
    curidx++;

    //store data in to pack
    memcpy(pack->data, buff + curidx, pack->size);

    return;  
} 

#endif