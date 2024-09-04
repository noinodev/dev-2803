#ifndef PROTOCOL
#define PROTOCOL

#include <stdint.h>
#include <stdio.h>

#define INPUT_MAX 256
#define PACKET_MAX 1024
#define PACKET_TEXT_PAYLOAD 64

#define HEADER_MOVE 1
#define HEADER_TEXT 2
#define HEADER_END 3
#define HEADER_ERROR 4

#define GAME_STATE_WAIT 0
#define GAME_STATE_GO 1

#define NETWORK_TARGET_TO 0
#define NETWORK_TARGET_EXCEPT 1
#define NETWORK_TARGET_ALL 2

typedef struct {
    int port;
    char *name;
} session;

typedef struct {
    uint8_t header;
    uint8_t move;
} packet_move;

typedef struct {
    char msg[PACKET_TEXT_PAYLOAD];
} packet_text;

#endif