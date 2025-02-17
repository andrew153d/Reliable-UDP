#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <sys/time.h>
#include "queue.h"
#include "rudp.h"

#define PAYLOADSIZE 1100
#define MAX_QUEUE_SIZE 10
#define MAX_SEND_RETRIES 5
#define PACKET_RESEND_TIMEOUT 500
#define PING_PERIOD 1000
#define ENABLE_PING 0

// utils
uint16_t CalculateChecksum(uint8_t *bytes, int byte_len);
long millis();

// Global variable to store the start time
struct timeval start_time;

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

struct MessageFrame
{
    struct DataPacket packet;
    struct sockaddr_in ep;
    uint8_t times_sent;
    long last_time_sent;
};

void SendPacket(struct DataPacket *packet, struct sockaddr_in ep);
void ReceiveHandler();
void Sender();
void SendAck(struct DataPacket out_packet);
/* Variables */
uint16_t local_port;
char *local_ip;
struct sockaddr_in local_addr;
int udp_socket;
struct sockaddr_in remote_addr;

bool incoming_num_initialized = false;
uint32_t incoming_data_index = 0;
uint32_t next_outgoing_index = 0;

uint32_t last_ping_receive_time;
uint32_t last_ping_send_time;

SessionState sessionState;
int (*OnBytesReceived)(uint8_t *, int) = NULL;

struct Queue packets;

/* End Variables */
void RUDP(const char *ip, int port)
{
    // Initialize the start time
    gettimeofday(&start_time, NULL);

    local_ip = (char *)malloc(strlen(ip) + 1);
    if (local_ip != NULL)
    {
        strcpy(local_ip, ip);
    }
    local_port = port;

    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_socket < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(local_port);

    if (inet_pton(AF_INET, local_ip, &local_addr.sin_addr) <= 0)
    {
        perror("Invalid IP address");
        close(udp_socket);
        exit(EXIT_FAILURE);
    }

    if (bind(udp_socket, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0)
    {
        perror("Bind failed");
        close(udp_socket);
        exit(EXIT_FAILURE);
    }

    printf("Listening on %s:%d for UDP packets...\n", local_ip, local_port);
    sessionState = CLOSED;
    srand(time(NULL));
    next_outgoing_index = abs(rand() % 1000);
    printf("Next outgoing index: %d\n", next_outgoing_index);
}

SessionState GetState()
{
    return sessionState;
}

void Connect(char *remote_ip, uint16_t remote_port)
{
    if (GetState() != CLOSED)
    {
        printf("Connection already established\n");
        return;
    }

    printf("Connecting to %s:%d\n", remote_ip, remote_port);
    memset(&remote_addr, 0, sizeof(remote_addr));
    remote_addr.sin_family = AF_INET;
    remote_addr.sin_port = htons(remote_port);
    inet_pton(AF_INET, remote_ip, &remote_addr.sin_addr);
    sessionState = OPENING;

    struct DataPacket packet;
    packet.header.type = SYN;
    packet.header.num = next_outgoing_index;
    packet.header.PayloadSize = 0;
    printf("Sending: %d:%d\n", packet.header.type, packet.header.num);
    SendPacket(&packet, remote_addr);
}

/**
 * @brief [Brief description of the function]
 *
 * Package the message in a DataPacket and use SendPacket to send
 *
 * @param [param1] [Description of the first parameter]
 * @param [param2] [Description of the second parameter]
 * @return [Description of the return value]
 */
int Send(uint8_t *bytes, int byte_len)
{
    if (byte_len > PAYLOADSIZE)
    {
        printf("Payload size too large\n");
        return -1;
    }

    if (queue_size(&packets) >= MAX_QUEUE_SIZE)
    {
        printf("Queue is full\n");
        return -1;
    }

    struct DataPacket p;
    p.header.type = DATA;
    p.header.num = next_outgoing_index;
    p.header.PayloadSize = byte_len;
    memcpy(p.Data, bytes, byte_len);

    if (queue_size(&packets) > 0)
    {
        struct MessageFrame *last = (struct MessageFrame *)queue_last(&packets);
        if (last == NULL)
        {
            printf("Error getting last packet\n");
            return -1;
        }
        p.header.num = last->packet.header.num + 2;
    }
    else
    {
        p.header.num = next_outgoing_index;
    }
    p.header.checksum = CalculateChecksum(p.Data, byte_len);

    SendPacket(&p, remote_addr);
}

void SendPacket(struct DataPacket *packet, struct sockaddr_in ep)
{
    struct MessageFrame *f = (struct MessageFrame *)malloc(sizeof(struct MessageFrame));
    f->packet = *packet;
    f->times_sent = 0;
    f->last_time_sent = 0;
    f->ep = remote_addr;
    queue_write(&packets, f);
}

void SendAck(struct DataPacket out_packet)
{
    printf("Sending ACK for %d\n", out_packet.header.num);
    out_packet.header.checksum = CalculateChecksum(out_packet.Data, out_packet.header.PayloadSize);
    sendto(udp_socket, &out_packet, sizeof(struct PacketHeader) + out_packet.header.PayloadSize, 0, (struct sockaddr *)&remote_addr, sizeof(remote_addr));
}

void Run()
{
    Sender();
    ReceiveHandler();
}

void Close()
{
    close(udp_socket);
}

void ReceiveHandler()
{
    char buffer[PAYLOADSIZE];
    struct sockaddr_in sender_addr;
    socklen_t addr_len = sizeof(sender_addr);
    ssize_t recv_len = recvfrom(udp_socket, buffer, PAYLOADSIZE, MSG_DONTWAIT, (struct sockaddr *)&sender_addr, &addr_len);
    if (recv_len < 0)
    {
        return;
    }

    struct DataPacket *receivedPacket = (struct DataPacket *)&buffer[0];
    printf("Received: %d:%d\n", receivedPacket->header.type, receivedPacket->header.num);
    last_ping_receive_time = millis();
    switch (receivedPacket->header.type)
    {
    case SYN:
    {
        if (sessionState != CLOSED)
        {
            // return;
        }

        remote_addr = sender_addr;
        sessionState = OPEN;
        incoming_data_index = receivedPacket->header.num;

        // Send back an ack
        struct DataPacket out_packet;
        out_packet.header.type = ACK;
        out_packet.header.num = incoming_data_index + 1;
        out_packet.header.PayloadSize = 0;
        SendAck(out_packet);
        break;
    }

    case DATA:
    {
        if(sessionState != OPEN)
        {
            //data on unopened socket
            return;
        }
        if(!incoming_num_initialized)
        {
            incoming_num_initialized = true;
            incoming_data_index = receivedPacket->header.num - 2;
        }
        if(incoming_data_index + 2 != receivedPacket->header.num)
        {
            printf("Mismatch indexes\n");
            return;
        }
        else
        {
            if(OnBytesReceived != NULL)
            {
                OnBytesReceived(receivedPacket->Data, receivedPacket->header.PayloadSize);
            }
        }
        incoming_data_index = receivedPacket->header.num;
        struct DataPacket out_packet;
        out_packet.header.type = ACK;
        out_packet.header.num = receivedPacket->header.num + 1;
        SendAck(out_packet);
        break;
    }

    case ACK:
    {
        if (queue_size(&packets) == 0)
        {
            return;
        }
        struct MessageFrame *thisPacket = (struct MessageFrame *)queue_peek(&packets);
        if (thisPacket == NULL)
        {
            printf("Error getting first packet\n");
            return;
        }
        if (thisPacket->packet.header.num + 1 == receivedPacket->header.num)
        {
            if (sessionState == OPENING)
            {
                sessionState = OPEN;
            }

            next_outgoing_index = receivedPacket->header.num + 1;

            thisPacket = queue_read(&packets);
        }
        break;
    }
    }
}

void Sender()
{
    if (ENABLE_PING)
    {
        if (millis() - last_ping_send_time > PING_PERIOD && GetState() == OPEN)
        {
            last_ping_send_time = millis();
            struct DataPacket packet;
            packet.header.PayloadSize = 0;
            packet.header.num = 0;
            packet.header.type = PING;
            packet.header.checksum = CalculateChecksum(packet.Data, packet.header.PayloadSize);
            printf("Sending: %d:%d\n", packet.header.type, packet.header.num);
            sendto(udp_socket, &packet, sizeof(struct PacketHeader) + packet.header.PayloadSize, 0, (struct sockaddr *)&remote_addr, sizeof(remote_addr));
        }

        if (millis() - last_ping_receive_time > PING_PERIOD * 3 && GetState() == OPEN)
        {
            last_ping_send_time = millis();
            printf("Lost comms");
            sessionState = CLOSED;
            Close();
            return;
        }
    }

    if (queue_size(&packets) == 0)
    {
        return;
    }
    struct MessageFrame *thisPacket = (struct MessageFrame *)queue_peek(&packets);
    if (thisPacket == NULL)
    {
        printf("Error getting first packet\n");
        return;
    }
    if (thisPacket->times_sent > MAX_SEND_RETRIES)
    {
        printf("Max retries reached\n");
        // TODO clear the buffer and exit
        sessionState = CLOSED;
        return;
    }
    if ((millis() - thisPacket->last_time_sent) > PACKET_RESEND_TIMEOUT)
    {
        thisPacket->times_sent++;
        thisPacket->last_time_sent = millis();
        printf("Sending: %d:%d\n", thisPacket->packet.header.type, thisPacket->packet.header.num);
        sendto(udp_socket, &thisPacket->packet, sizeof(struct PacketHeader) + thisPacket->packet.header.PayloadSize, 0, (struct sockaddr *)&thisPacket->ep, sizeof(thisPacket->ep));
    }
}

uint16_t CalculateChecksum(uint8_t *bytes, int byte_len)
{
    if (bytes == NULL)
        return 0;

    uint16_t checksum = 0;
    for (int i = 0; i < byte_len; i++)
    {
        checksum += bytes[i];
    }
    return checksum;
}

long millis()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long elapsed = (tv.tv_sec - start_time.tv_sec) * 1000 + (tv.tv_usec - start_time.tv_usec) / 1000;
    return elapsed;
}
