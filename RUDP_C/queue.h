#pragma once

#include <stdlib.h>
#include <stdio.h>


#define QUEUE_SIZE 10

struct Queue {
  void *items[QUEUE_SIZE];
  int read_index;
  int write_index;
};

void queue_init(struct Queue *q);
void* queue_read(struct Queue *q);
int queue_write(struct Queue *q, void *item);
int queue_size(struct Queue *q);
void* queue_peek(struct Queue *q);
void* queue_last(struct Queue *q);