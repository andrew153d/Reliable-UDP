#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "circ_buf.h"

void ringbuf_init(rbuf_t* _this)
{
  // clear the _thisfer and init the values
  // and sets head = tail in one go
  memset( _this, 0, sizeof(*_this) );
}

bool ringbuf_empty(rbuf_t* _this)
{
  // test if the queue is empty
  // 0 returns true
  // nonzero false
  return (0 == _this->count);
}

bool ringbuf_full(rbuf_t* _this)
{
  // full when no of elements exceed the max size
  return (_this->count >= RBUF_SIZE);
}

MessageFrame ringbuf_get(rbuf_t* _this)
{
  MessageFrame item;
  if (_this->count > 0)
  {
    // get item element
    item        = _this->buf[_this->tail];
    // advance the tail
    _this->tail = ringbuf_adv(_this->tail, RBUF_SIZE);
    // reduce the total count
    --_this->count;
  }
  else {
    // the queue is empty
    //item = RBUF_EMPTY;
  }
  return item;
}

void ringbuf_put(rbuf_t* _this, MessageFrame item)
{
  if (_this->count < RBUF_SIZE)
  {
    // set the item at head position
    _this->buf[_this->head] = item;
    // advance the head
    _this->head = ringbuf_adv(_this->head, RBUF_SIZE);
    // increase the total count
    ++_this->count;
  }
}

MessageFrame* ringbuf_peek(rbuf_t* _this)
{
  if (_this->count != 0)
  {
	return &_this->buf[_this->tail];
  
  }
  return NULL;
}

void ringbuf_flush(rbuf_t* _this, rbuf_opt_e clear)
{
  _this->count = 0;
  _this->head  = 0;
  _this->tail  = 0;

  // optionally clear
  if (RBUF_CLEAR == clear)
  {
    memset(_this->buf, 0, sizeof(_this->buf));
  }
}

unsigned int ringbuf_adv(const unsigned int value, const unsigned int max)
{
  unsigned int index = value + 1;
  if (index >= max)
  {
    index = 0;
  }
  return index;
}