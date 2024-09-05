#ifndef PROTOCOL
#define PROTOCOL

#include <stdint.h>
#include <stdio.h>

#define INPUT_MAX 256
#define PACKET_MAX 128
#define PACKET_MIN 2

//#define HEADER_PING 6
#define HEADER_MOVE 1
#define HEADER_TEXT 2
#define HEADER_END 3
#define HEADER_ERROR 4
#define HEADER_INFO 5

#define GAME_STATE_WAIT 0
#define GAME_STATE_GO 1
#define GAME_INFRACTION_LIMIT 5
#define GAME_MIN_PLAYERS 2

#define NETWORK_TARGET_TO 0
#define NETWORK_TARGET_EXCEPT 1
#define NETWORK_TARGET_ALL 2
#define NETWORK_TIMEOUT_PING 10
#define NETWORK_TIMEOUT_WAIT 20

#define GAME_ERROR_OOB 0
#define GAME_ERROR_SEQ 1
extern const char* err_game[];

typedef struct {
    char buffer[PACKET_MAX];
    int pos;
} net_buffer;

net_buffer buffer_create();
void buffer_seek(net_buffer* buff, int seek);
int buffer_tell(net_buffer* buff);
void* buffer_read(net_buffer* buff, int size);
void buffer_read_string(net_buffer* buff, char* string);
void buffer_write(net_buffer* buff, void* data, int size);
void buffer_write_string(net_buffer* buff, char* string);

typedef struct {
    int state,socket,terminate,ping;
    char name[32];
} clientdata;

//void network_disconnect(int socket);

#endif