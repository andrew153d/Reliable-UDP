#include "rudp.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

void HandleReceivedBytes(const unsigned char *bytes, size_t length) {
    for (size_t i = 0; i < length; i++) {
        printf("%02X ", bytes[i]);
    }
    printf("\n");
}

int main() {
    printf("Starting the Client\n");

    const char *filePath = "firmware.txt";
    FILE *file = fopen(filePath, "rb");
    if (!file) {
        perror("Failed to open file");
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);

    unsigned char *data = (unsigned char *)malloc(fileSize);
    if (!data) {
        perror("Failed to allocate memory");
        fclose(file);
        return 1;
    }

    fread(data, 1, fileSize, file);
    fclose(file);

    int send_index = 0;

    rudp_init("127.0.0.1", 15680);
    rudp_connect("127.0.0.1", 15671);
    while (rudp_get_state() != OPEN) {
        rudp_run();
    }
    
    int send_size = 1000;

    while (rudp_get_state() != CLOSED) {
        rudp_run();

        // Send the data in chunks of 'send_size', adjusting to ensure you send the remaining data
        int remainingData = fileSize - send_index;
        int currentSendSize = remainingData < send_size ? remainingData : send_size;

        if (rudp_send(data + send_index, currentSendSize) != -1) {
            send_index += currentSendSize;
            printf("Sent %d of %ld bytes | %.2f%%\n", send_index, fileSize, ((float)send_index / (float)fileSize) * 100);
            if (send_index == fileSize) {
                break;
            }
        }
    }
    while (rudp_get_state() != CLOSED) {
        rudp_run();
    }

    free(data);
    return 0;
}
