#pragma once
#include "stdint.h"

typedef enum
{
    OPENING,
    OPEN,
    CLOSE,
    CLOSED
} SessionState;

void RUDP(const char *ip, int port);
void Connect(char *remote_ip, uint16_t remote_port);
int Send(uint8_t* bytes, int byte_len);
SessionState GetState();

void Run();
void Close();