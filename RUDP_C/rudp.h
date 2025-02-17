#pragma once
#include "stdint.h"

typedef enum
{
    OPENING,
    OPEN,
    CLOSE,
    CLOSED
} SessionState;

void rudp_init(const char *ip, int port);
void rudp_connect(char *remote_ip, uint16_t remote_port);
int rudp_send(uint8_t* bytes, int byte_len);
SessionState rudp_get_state();
void set_recv_callback(void (*callback)(unsigned char *, int));
void rudp_run();
void rudp_close();