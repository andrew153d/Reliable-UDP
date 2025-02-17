#include "queue.h"

void queue_init(struct Queue *q) {
  q->read_index = 0;
  q->write_index = 0;
}

void* queue_read(struct Queue *q) {
  if (q->read_index == q->write_index) {
    return NULL;
  }
  void *item = q->items[q->read_index];
  q->read_index = (q->read_index + 1) % QUEUE_SIZE;
  return item;
}

int queue_write(struct Queue *q, void *item) {
  if ((q->write_index + 1) % QUEUE_SIZE == q->read_index) {
    return -1;
  }
  q->items[q->write_index] = item;
  q->write_index = (q->write_index + 1) % QUEUE_SIZE;
  return 1;
}

int queue_size(struct Queue *q) {
  return (q->write_index - q->read_index + QUEUE_SIZE) % QUEUE_SIZE;
}

void* queue_last(struct Queue *q){
  if (q->read_index == q->write_index) {
    return NULL;
  }
  return q->items[(q->write_index - 1 + QUEUE_SIZE) % QUEUE_SIZE];
}

void* queue_peek(struct Queue *q)
{
  if(queue_size(q) == 0)
  {
    return NULL;
  }
  return q->items[q->read_index];
}