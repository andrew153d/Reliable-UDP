#include "rudp.h"
#include <stdlib.h>
#include <stdio.h>
#include "queue.h"
#include <stdint.h>
#include <string.h>

void HandleReceivedBytes(unsigned char* bytes, int length) {
  FILE *file = fopen("firmware1.txt", "ab");
  if (file != NULL) {
    fwrite(bytes, sizeof(unsigned char), length, file);
    fclose(file);
  }
}

int main() {
    FILE *file = fopen("firmware1.txt", "w");
    if (file != NULL) {
        fclose(file);
    }

    rudp_init("127.0.0.1", 15671);
    set_recv_callback(HandleReceivedBytes);
    while(rudp_get_state()!=OPEN)
    {
      rudp_run();
    }
    while(rudp_get_state()!=CLOSED)
    {
      rudp_run();
    }
    return 0;
}
