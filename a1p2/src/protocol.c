#include "protocol.h"
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

// error strings
const char* err_game[] = {"move out of bounds and you shouldn't be able to do that","it is not your turn and you shouldn't be able to do that"};

// buffer functions for serialized read/write of network data, using fixed size buffers

// reset and return to the start of the buffer, seek in real life doesnt actually clear the buffer but i felt like doing that
void buffer_seek(net_buffer* buff, int seek){
    memset(buff->buffer, '\0', PACKET_MAX*sizeof(char));
    buff->pos = seek;
}

// return current position (effective size) of buffer
int buffer_tell(net_buffer* buff){
    return buff->pos;
}

// get next n bytes of buffer
void* buffer_read(net_buffer* buff, int size){
    void* data = &(buff->buffer[buff->pos]);
    buff->pos += size;
    return data;
}

// read string from buffer until \0 terminating char
void buffer_read_string(net_buffer* buff, char* string){
    strncpy(string,buff->buffer+buff->pos,INPUT_MAX*sizeof(char));
    buff->pos += strlen(string);
}

// write to next n bytes of buffer
void buffer_write(net_buffer* buff, void* data, int size){
    memcpy(&(buff->buffer[buff->pos]),data,size);
    buff->pos += size;
    buff->buffer[buff->pos] = '\0';
}   

// write string to buffer
void buffer_write_string(net_buffer* buff, char* string){
    strncpy(buff->buffer+buff->pos,string,PACKET_MAX-buff->pos);
    buff->pos += (strlen(string))*sizeof(char);
    buff->buffer[buff->pos] = '\0';
}