#pragma once
#include "stdint.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <sys/time.h>
#include <time.h>
#include "circ_buf.h"
#include "defines.h"



struct rudp_session * rudp_init(struct rudp_session* session, const char *ip, int port);
void rudp_connect(struct rudp_session* session,char *remote_ip, uint16_t remote_port);
int rudp_send(struct rudp_session* session,uint8_t* bytes, int byte_len);
SessionState rudp_get_state(struct rudp_session* session);
void set_recv_callback(struct rudp_session* session, void (*callback)(unsigned char *, int));
void rudp_run(struct rudp_session* session);
void rudp_close(struct rudp_session* session);