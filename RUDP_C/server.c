#include "rudp.h"
#include <stdlib.h>
#include <stdio.h>
#include "queue.h"



int main() {

    RUDP("127.0.0.1", 15671);
    while(GetState()!=OPEN)
    {
      Run();
    }
    while(GetState()!=CLOSED)
    {
      Run();
    }
    return 0;
}
