#include "rudp.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

void HandleReceivedBytes(unsigned char* bytes, int length) {
  FILE *file = fopen("firmware1.txt", "ab");
  if (file != NULL) {
    fwrite(bytes, sizeof(unsigned char), length, file);
    fclose(file);
  }
}

struct rudp_session *session;

int main() {
    FILE *file = fopen("firmware1.txt", "w");
    if (file != NULL) {
        fclose(file);
    }

    session = rudp_init(session, "100.91.81.117", 15671);
    set_recv_callback(session, HandleReceivedBytes);
    while(rudp_get_state(session)!=OPEN)
    {
      rudp_run(session);
    }
    while(rudp_get_state(session)!=CLOSED)
    {
      rudp_run(session);
    }
    return 0;
}
