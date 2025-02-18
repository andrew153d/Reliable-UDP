#include "rudp.h"

uint16_t calculate_checksum(uint8_t *bytes, int byte_len);
uint32_t millis();
time_t sec;
void send_packet(struct rudp_session* session, struct DataPacket *packet);
void receive_packets( struct rudp_session* session);
void send_packets(struct rudp_session* session);
void send_ack(struct rudp_session* session, struct DataPacket out_packet);

struct timeval start_time;
struct rudp_session * rudp_init(struct rudp_session* session, const char *ip, int port)
{
    session = (struct rudp_session*)malloc(sizeof(struct rudp_session));
    gettimeofday(&start_time, NULL);
    session->local_ip = (char *)malloc(strlen(ip) + 1);
    if (session->local_ip != NULL)
    {
        strcpy(session->local_ip, ip);
    }
    session->local_port = port;

    session->udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (session->udp_socket < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&session->local_endpoint, 0, sizeof(session->local_endpoint));
    session->local_endpoint.sin_family = AF_INET;
    session->local_endpoint.sin_port = htons(session->local_port);

    if (inet_pton(AF_INET, session->local_ip, &session->local_endpoint.sin_addr) <= 0)
    {
        perror("Invalid IP address");
        close(session->udp_socket);
        exit(EXIT_FAILURE);
    }

    if (bind(session->udp_socket, (struct sockaddr *)&session->local_endpoint, sizeof(session->local_endpoint)) < 0)
    {
        perror("Bind failed");
        close(session->udp_socket);
        exit(EXIT_FAILURE);
    }

    printf("Listening on %s:%d for UDP packets...\n", session->local_ip, session->local_port);
    session->sessionState = CLOSED;
    
    session->next_outgoing_index = abs(rand() % 1000);
    ringbuf_init(&session->packets);
    return session;
}

SessionState rudp_get_state(struct rudp_session* session)
{
    return session->sessionState;
}

void rudp_connect(struct rudp_session* session, char *remote_ip, uint16_t remote_port)
{
    // printf("Connecting\n");
    // MessageFrame frame;
    // frame.times_sent = 53;
    // frame.last_time_sent = 0;
    // frame.ep = session->remote_endpoint;
    
    // circular_buf_put(session->packets, frame);
    // printf("Times sent: %d, Last Time Sent: %d, Millis: %ld\n", frame.times_sent, frame.last_time_sent, millis());
    
    // MessageFrame thisPacket;
    // if (circular_buf_peek(session->packets, &thisPacket, 0)!=0)
    // {
    //     printf("Error getting first packet\n");
    //     while(1);
    // }
    
    // printf("Times sent: %d, Last Time Sent: %d, Millis: %ld\n", thisPacket.times_sent, thisPacket.last_time_sent, millis());

    
    // while(1);

    if (rudp_get_state(session) != CLOSED)
    {
        printf("Connection already established\n");
        return;
    }


    printf("Connecting to %s:%d\n", remote_ip, remote_port);
    memset(&session->remote_endpoint, 0, sizeof(session->remote_endpoint));
    session->remote_endpoint.sin_family = AF_INET;
    session->remote_endpoint.sin_port = htons(remote_port);
    inet_pton(AF_INET, remote_ip, &session->remote_endpoint.sin_addr);
    session->sessionState = OPENING;

    struct DataPacket packet;
    packet.header.type = SYN;
    packet.header.num = session->next_outgoing_index;
    session->next_outgoing_index=session->next_outgoing_index+2;
    packet.header.PayloadSize = 0;
    //printf("Sgending: %d:%d\n", packet.header.type, packet.header.num);
    send_packet(session, &packet);
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
int rudp_send(struct rudp_session* session, uint8_t *bytes, int byte_len)
{
    if (byte_len > PAYLOADSIZE)
    {
        printf("Payload size too large\n");
        return -1;
    }

    if (ringbuf_full(&session->packets))
    {
        //printf("Queue is full\n");
        return -1;
    }

    struct DataPacket p;
    p.header.type = DATA;
    p.header.num = session->next_outgoing_index;
    session->next_outgoing_index = session->next_outgoing_index+2;
    p.header.PayloadSize = byte_len;
    memcpy(p.Data, bytes, byte_len);

    // if (!ringbuf_empty(&session->packets))
    // {
    //     // struct MessageFrame *last = (struct MessageFrame *)queue_last(&session->packets);
    //     // if (last == NULL)
    //     // {
    //     //     printf("Error getting last packet\n");
    //     //     return -1;
    //     // }
    //     // p.header.num = last->packet.header.num + 2;
    //     p.header.num = session->next_outgoing_index + 2;
    // }
    // else
    // {
    //     p.header.num = session->next_outgoing_index;
    // }
    p.header.checksum = calculate_checksum(p.Data, byte_len);
    send_packet(session, &p);
    return 1;
}

void send_packet(struct rudp_session* session, struct DataPacket *packet)
{
    MessageFrame f;
    f.packet = *packet;
    f.times_sent = 0;
    f.last_time_sent = 0;
    f.ep = session->remote_endpoint;
    f.last_time_sent = millis();
    printf("Putting packet num %d\n", f.packet.header.num);
    ringbuf_put(&session->packets, f);
    //circular_buf_put(session->packets, (void**)&f);
}

void send_ack(struct rudp_session* session, struct DataPacket out_packet)
{
    out_packet.header.checksum = calculate_checksum(out_packet.Data, out_packet.header.PayloadSize);
    printf("Sending: %d:%d\n", out_packet.header.type, out_packet.header.num);
    sendto(session->udp_socket, &out_packet, sizeof(struct PacketHeader) + out_packet.header.PayloadSize, 0, (struct sockaddr *)&session->remote_endpoint, sizeof(session->remote_endpoint));
}

void set_recv_callback(struct rudp_session* session, void (*callback)(unsigned char *, int))
{
    session->OnBytesReceived = callback;
}

void rudp_run(struct rudp_session* session)
{
    send_packets(session);
    receive_packets(session);
}

void rudp_close(struct rudp_session* session)
{
    close(session->udp_socket);
}

void receive_packets(struct rudp_session* session)
{
    char buffer[PAYLOADSIZE];
    struct sockaddr_in sender_addr;
    socklen_t addr_len = sizeof(sender_addr);
    ssize_t recv_len = recvfrom(session->udp_socket, buffer, PAYLOADSIZE, MSG_DONTWAIT, (struct sockaddr *)&sender_addr, &addr_len);
    if (recv_len < 0)
    {
        return;
    }

    struct DataPacket *receivedPacket = (struct DataPacket *)&buffer[0];

    uint16_t receivedChecksum = calculate_checksum(receivedPacket->Data, receivedPacket->header.PayloadSize);
    if (receivedChecksum != receivedPacket->header.checksum)
    {
        printf("Checksum mismatch: %d | %d\n", receivedChecksum, receivedPacket->header.checksum);
        // return;
    }

    printf("Received: %d:%d\n", receivedPacket->header.type, receivedPacket->header.num);
    session->last_ping_receive_time = millis();
    switch (receivedPacket->header.type)
    {
    case SYN:
    {
        if (session->sessionState != CLOSED)
        {
            // return;
        }

        session->remote_endpoint = sender_addr;
        session->sessionState = OPEN;
        session->incoming_data_index = receivedPacket->header.num;

        // Send back an ack
        struct DataPacket out_packet;
        out_packet.header.type = ACK;
        out_packet.header.num = session->incoming_data_index + 1;
        out_packet.header.PayloadSize = 0;
        send_ack(session, out_packet);
        break;
    }

    case DATA:
    {
        if (session->sessionState != OPEN)
        {
            // data on unopened socket
            return;
        }
        if (!session->incoming_num_initialized)
        {
            session->incoming_num_initialized = true;
            session->incoming_data_index = receivedPacket->header.num - 2;
        }
        if (session->incoming_data_index + 2 != receivedPacket->header.num)
        {
            // printf("Mismatch indexes\n");
            // return;
        }
        else
        {
            if (session->OnBytesReceived != NULL)
            {
                session->OnBytesReceived(receivedPacket->Data, receivedPacket->header.PayloadSize);
            }
        }
        session->incoming_data_index = receivedPacket->header.num;
        struct DataPacket out_packet;
        out_packet.header.type = ACK;
        out_packet.header.num = receivedPacket->header.num + 1;
        send_ack(session, out_packet);
        break;
    }

    case ACK:
    {
        if (ringbuf_empty(&session->packets))
        {
            printf("Received an ack on an empty buffer");
            return;
        }
        MessageFrame* thisPacket = ringbuf_peek(&session->packets);
        if (thisPacket == NULL)
        {
            printf("Error getting first packet\n");
            return;
        }
        if (thisPacket->packet.header.num + 1 == receivedPacket->header.num)
        {
            if (session->sessionState == OPENING)
            {
                printf("Setting state to open\n");
                session->sessionState = OPEN;
            }

            //session->next_outgoing_index = receivedPacket->header.num + 1;
            printf("Removing from buffer: %d\n", thisPacket->packet.header.num);
            (void)ringbuf_get(&session->packets);
        }
        break;
    }
    case FIN:
        break;
    case RAW_UDP:
        break;
    case PING:
        break;
    }
}

void send_packets(struct rudp_session* session)
{
    if (ENABLE_PING)
    {
        if (millis() - session->last_ping_send_time > PING_PERIOD && rudp_get_state(session) == OPEN)
        {
            session->last_ping_send_time = millis();
            struct DataPacket packet;
            packet.header.PayloadSize = 0;
            packet.header.num = 0;
            packet.header.type = PING;
            packet.header.checksum = calculate_checksum(packet.Data, packet.header.PayloadSize);
            printf("Sending: %d:%d\n", packet.header.type, packet.header.num);
            sendto(session->udp_socket, &packet, sizeof(struct PacketHeader) + packet.header.PayloadSize, 0, (struct sockaddr *)&session->remote_endpoint, sizeof(session->remote_endpoint));
        }

        if (millis() - session->last_ping_receive_time > PING_PERIOD * 3 && rudp_get_state(session) == OPEN)
        {
            session->last_ping_send_time = millis();
            printf("Lost comms");
            session->sessionState = CLOSED;
            rudp_close(session);
            return;
        }
    }
    if (ringbuf_empty(&session->packets))
    {
        return;
    }

    MessageFrame* thisPacket = ringbuf_peek(&session->packets);
    if (thisPacket == NULL)
    {
        printf("Error getting first packet\n");
        return;
    }

    if (thisPacket->times_sent > MAX_SEND_RETRIES)
    {
        printf("Max retries reached: %d   Last Time: %ld\n", thisPacket->times_sent, thisPacket->last_time_sent);
        while(1);
        // TODO clear the buffer and exit
        session->sessionState = CLOSED;
        return;
    }

    
    if ((millis() - thisPacket->last_time_sent) > PACKET_RESEND_TIMEOUT || thisPacket->times_sent==0)
    {
        printf("Millis: %ld | Last time: %ld\n", millis(), thisPacket->last_time_sent);
        thisPacket->times_sent++;
        thisPacket->last_time_sent = millis();
        printf("Sending: %d:%d\n", thisPacket->packet.header.type, thisPacket->packet.header.num);
        sendto(session->udp_socket, &thisPacket->packet, sizeof(struct PacketHeader) + thisPacket->packet.header.PayloadSize, 0, (struct sockaddr *)&thisPacket->ep, sizeof(thisPacket->ep));
    }
}

uint16_t calculate_checksum(uint8_t *bytes, int byte_len)
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




uint32_t millis()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long elapsed = (tv.tv_sec - start_time.tv_sec) * 1000 + (tv.tv_usec - start_time.tv_usec) / 1000;
    return elapsed;
}
