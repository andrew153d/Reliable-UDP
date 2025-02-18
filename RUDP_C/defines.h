#ifndef DEFINES_H
#define DEFINES_H

#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PAYLOADSIZE 1000
#define RBUF_SIZE 8

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

struct PacketHeader
{
    MessageType type;
    uint16_t PayloadSize;
    uint16_t checksum;
    uint32_t num;
};

struct DataPacket
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

#endif