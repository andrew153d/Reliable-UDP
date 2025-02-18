CC = gcc
CFLAGS = -Wall -Wextra -g

SERVER_SRCS = server.c rudp.c circ_buf.c
CLIENT_SRCS = client.c rudp.c circ_buf.c
CIRC_SRCS = buftest.c circ_buf.c

SERVER_OBJS = $(SERVER_SRCS:.c=.o)
CLIENT_OBJS = $(CLIENT_SRCS:.c=.o)
CIRC_OBJS = $(CIRC_SRCS:.c=.o)

HEADERS = rudp.h circ_buf.h defines.h

SERVER_TARGET = server
CLIENT_TARGET = client
CIRC_TARGET = buftest

all: $(SERVER_TARGET) $(CLIENT_TARGET)

$(SERVER_TARGET): $(SERVER_OBJS)
	$(CC) $(CFLAGS) -o $(SERVER_TARGET) $(SERVER_OBJS)

$(CLIENT_TARGET): $(CLIENT_OBJS)
	$(CC) $(CFLAGS) -o $(CLIENT_TARGET) $(CLIENT_OBJS)

$(CIRC_TARGET): $(CIRC_OBJS)
	$(CC) $(CFLAGS) -o $(CIRC_TARGET) $(CIRC_OBJS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(SERVER_OBJS) $(CLIENT_OBJS) $(SERVER_TARGET) $(CLIENT_TARGET) $(CIRC_OBJS) $(CIRC_TARGET)

.PHONY: all clean