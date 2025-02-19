#ifndef DEFINES_H
#define DEFINES_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "stdint.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <sys/time.h>
#include <time.h>

#define PAYLOADSIZE 1000
#define RBUF_SIZE 8
#define MAX_QUEUE_SIZE 10
#define MAX_SEND_RETRIES 10
#define PACKET_RESEND_TIMEOUT 1000
#define PING_PERIOD 1000
#define ENABLE_PING 0

typedef enum
{
    OPENING,
    OPEN,
    CLOSE,
    CLOSED
} SessionState;

typedef enum
{
    SYN,
    ACK,
    DATA,
    FIN,
    RAW_UDP,
    PING
} MessageType;

struct __attribute__((__packed__)) PacketHeader
{
    uint8_t type;
    uint16_t PayloadSize;
    uint16_t checksum;
    uint32_t num;
};

struct __attribute__((__packed__)) DataPacket
{
    struct PacketHeader header;
    uint8_t Data[PAYLOADSIZE];
};

typedef struct MessageFramet
{
    struct DataPacket packet;
    struct sockaddr_in ep;
    uint8_t times_sent;
    long last_time_sent;
}MessageFrame;

typedef struct ring_buf_s
{
  MessageFrame buf[RBUF_SIZE];
  int head;
  int tail;
  int count;
} rbuf_t;

struct rudp_session
{
    uint16_t local_port;
    char *local_ip;
    struct sockaddr_in local_endpoint;
    struct sockaddr_in remote_endpoint;
    int udp_socket;
    bool incoming_num_initialized;
    uint32_t incoming_data_index;
    uint32_t next_outgoing_index;
    uint32_t last_ping_receive_time;
    uint32_t last_ping_send_time;
    SessionState sessionState;
    void (*OnBytesReceived)(uint8_t *, int);
    rbuf_t packets;
};

struct rudp_session * rudp_init(struct rudp_session* session, const char *ip, int port);
void rudp_connect(struct rudp_session* session,char *remote_ip, uint16_t remote_port);
int rudp_send(struct rudp_session* session,uint8_t* bytes, int byte_len);
SessionState rudp_get_state(struct rudp_session* session);
void set_recv_callback(struct rudp_session* session, void (*callback)(unsigned char *, int));
void rudp_run(struct rudp_session* session);
void rudp_close(struct rudp_session* session);

#endif