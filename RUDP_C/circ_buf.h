#ifndef RING_BUF_H
#define RING_BUF_H

#include <stdbool.h>
#include "rudp.h"

typedef enum
{
  RBUF_CLEAR,
  RBUF_NO_CLEAR
} rbuf_opt_e;

typedef enum
{
  RBUF_EMPTY = -1,
  RBUF_FULL
} rbuf_msg_e;

  void ringbuf_init(rbuf_t* _this);
  bool ringbuf_empty(rbuf_t* _this);
  bool ringbuf_full(rbuf_t* _this);
  MessageFrame ringbuf_get(rbuf_t* _this);
  void ringbuf_put(rbuf_t* _this, MessageFrame item);
  MessageFrame* ringbuf_peek(rbuf_t* _this);
  void ringbuf_flush(rbuf_t* _this, rbuf_opt_e clear);
  unsigned int ringbuf_adv (const unsigned int value, const unsigned int max_val);

#endif