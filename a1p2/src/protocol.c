#include "protocol.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

const char* err_game[] = {"move out of bounds and you shouldn't be able to do that","it is not your turn and you shouldn't be able to do that"};

net_buffer buffer_create(){
    net_buffer buff;
    memset(buff.buffer, '\0', PACKET_MAX*sizeof(char));
    buff.pos = 0;
    return buff;
}

void buffer_seek(net_buffer* buff, int seek){
    memset(buff->buffer, '\0', PACKET_MAX*sizeof(char));
    buff->pos = seek;
}

int buffer_tell(net_buffer* buff){
    return buff->pos;
}

void* buffer_read(net_buffer* buff, int size){
    void* data = &(buff->buffer[buff->pos]);
    buff->pos += size;
    return data;
}

void buffer_read_string(net_buffer* buff, char* string){
    strncpy(string,buff->buffer+buff->pos,INPUT_MAX*sizeof(char));
    buff->pos += strlen(string);
}

void buffer_write(net_buffer* buff, void* data, int size){
    memcpy(&(buff->buffer[buff->pos]),data,size);
    buff->pos += size;
    buff->buffer[buff->pos] = '\0';
}   

void buffer_write_string(net_buffer* buff, char* string){
    strncpy(buff->buffer+buff->pos,string,PACKET_MAX-buff->pos);
    buff->pos += (strlen(string))*sizeof(char);
    buff->buffer[buff->pos] = '\0';
}

/*void network_disconnect(int socket){
    
    char buffer[PACKET_MIN];
    buffer[0] = HEADER_END;
    printf("forcing disconnect\n");
    send(socket,buffer,PACKET_MIN,0);
}*/