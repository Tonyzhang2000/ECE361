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
    //filedata好像会出锅，但我不知道为啥

    int headerLen = sprintf(buff, "%d:%d:%d:%s:", pack->total_frag, pack->frag_no, pack->size, pack->filename);

    memcpy(buff + headerLen, pack->filedata, sizeof(char) * 1000);

    return headerLen;
}


//store the information in buff back to the struct packet
void deserialize(struct packet *pack, const char *buff) {
    return;
} 

#endif