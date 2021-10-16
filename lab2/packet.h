#ifndef PACKET_H
#define PACKET_H

struct packet{
    unsigned int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char* filename;
    char filedata[1000];
};

//store packet information into the string buff, and return the header length
int serialize(const struct packet *pack, char *buff) {

    //clear the string
    memset(buff, 0, 2000);
    //写了个寂寞，但如果是最后一个packet估计不能这样吧
    //can't use sprint for filedata, this is because some bianry file may include special char like '\0

    int headerLen = sprintf(buff, "%d:%d:%d:%s:", pack->total_frag, pack->frag_no, pack->size, pack->filename);

    memcpy(buff + headerLen, pack->filedata,  1000);

    return headerLen;
}

//store the information in buff back to the struct packet
void deserialize(struct packet *pack, const char *buff) {

    char str[100];
    int curidx = 0, len = 0;

    //store total frag into the pack
    for(int i = 0; ;i++) {
        if(buff[i+curidx] == ':') {
            len = i;
            break;
        }
    }
    memset(str, 0, sizeof(str));
    memcpy(str, buff + curidx, len);
    pack->total_frag = atoi(str);
    curidx += len;
    curidx++;

    //store data size into the pack
    for(int i = 0; ;i++) {
        if(buff[i+curidx] == ':') {
            len = i;
            break;
        }
    }
    memset(str, 0, sizeof(str));
    memcpy(str, buff + curidx, len);
    pack->frag_no = atoi(str);
    curidx += len;
    curidx++;

    //store current frag into the pack
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

    //store file name into the pack
    for(int i = 0; ;i++) {
        if(buff[i+curidx] == ':') {
            len = i;
            break;
        }
    }
    memset(str, 0, sizeof(str));
    memcpy(str, buff + curidx, len);
    printf("@@@%s@@@\n", str);
    //memset(pack->filename, 0, sizeof(pack->filename));
    strcpy(pack->filename, str);
    //pack->filename = "";
    curidx += len; 
    curidx++;

    //store file data in to pack
    memset(str, 0, sizeof(str));
    memcpy(pack->filedata, buff + curidx, pack->size);

    //print packet message, for debug usage
    printf("totalfrag = %d\n", pack->total_frag);
    printf("currentfrag = %d\n", pack->frag_no);
    printf("size = %d\n", pack->size);
    printf("filename = %s\n", pack->filename);
    printf("filedata = %s\n", pack->filedata);

    return;  
} 

#endif