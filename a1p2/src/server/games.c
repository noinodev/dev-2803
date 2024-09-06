#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/utsname.h>
#include <time.h>
#include <netdb.h>
#include <string.h>
#include <math.h>
#include "../protocol.h"
#include "server.h"

// check for errors in input for numbers
int game_numbers_move_check(threadcommon* common, char* string){
    int error = 0;
    char *errptr;
    double out = strtod(string,&errptr);
    if(*errptr != '\0' || out != (char)out) error |= GAME_ERROR_MAL;
    if((char)out != (char)fmin(fmax(out,1),9)) error |= GAME_ERROR_OOB;
    return error;
}

// numbers update function -> decrement value and send message to next player
int game_numbers_move_update(threadcommon* common, char* string){
    char *errptr;
    char out = (char)strtod(string,&errptr);
    common->game.data[1] -= out;

    net_buffer buffer_send;
    buffer_seek(&buffer_send,0);
    char hout = HEADER_TEXT, string_send[INPUT_MAX];
    snprintf(string_send,INPUT_MAX*sizeof(char),"current value is %i. enter a number 0-9",common->game.data[1]);
    buffer_write(&buffer_send,&hout,sizeof(char));
    buffer_write_string(&buffer_send,string_send);
    send(common->sockets->data.socket,buffer_send.buffer,buffer_tell(&buffer_send),0);

    if(common->game.data[1] <= 0) return 1;
    return 0;
}

// this whole thing is just kind of to illustrate a point

int game_rps_move_check(threadcommon* common, char* string){
    int error = 0;
    if(strcmp(string,"rock") > 0 && strcmp(string,"paper") > 0 && strcmp(string,"scissors") > 0) error |= GAME_ERROR_OOB;
    return error;
}

int game_rps_move_update(threadcommon* common, char* string){
    char move = 0, w = 0;
    if(strcmp(string,"rock") == 0) move = 1;
    if(strcmp(string,"paper") == 0) move = 2;
    if(strcmp(string,"scissors") == 0) move = 3;
    if(common->game.data[1] == 0){
        w = 0;
    }else if(common->game.data[1] == move){
        w = 3;
    }else{
        char last = common->game.data[1];
        if(move == 1 && last == 3) w = 1;
        if(move == 2 && last == 1) w = 1;
        if(move == 3 && last == 2) w = 1;

        if(last == 1 && move == 3) w = 2;
        if(last == 2 && move == 1) w = 2;
        if(last == 3 && move == 2) w = 2;
    }
    common->game.data[1] = move;

    net_buffer buffer_send;
    buffer_seek(&buffer_send,0);
    char hout = HEADER_TEXT, string_send[INPUT_MAX];
    char* msg[] = {"opponent went first\n","opponent beat you\n","you beat opponent\n","you tied\n"};
    buffer_write(&buffer_send,&hout,sizeof(char));
    buffer_write_string(&buffer_send,msg[w]);
    snprintf(string_send,INPUT_MAX*sizeof(char),"enter 'rock' 'paper' or 'scissors'");
    buffer_write(&buffer_send,&hout,sizeof(char));
    buffer_write_string(&buffer_send,string_send);
    send(common->sockets->data.socket,buffer_send.buffer,buffer_tell(&buffer_send),0);
    return 0;
}