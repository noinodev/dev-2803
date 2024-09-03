#ifndef PROTOCOL
#define PROTOCOL

#include <stdint.h>
#include <stdio.h>

#define PKT_

typedef struct {
    uint8_t header;
    uint32_t time;
    uint16_t length;
    char* payload;
} packet;

#endif