CC = gcc
CFLAGS = -Wall -Wextra -Iinclude
DBGFLAGS = -DDEBUG -g

SRCS = main.c \
       serial.c \
       proto.c \
       cmd.c \
       req.c \
       util.c

OUT = fanctl

all: $(OUT)

$(OUT): $(SRCS)
	$(CC) $(CFLAGS) -I. -o $@ $(SRCS)

debug: $(SRCS)
	$(CC) $(CFLAGS) $(DBGFLAGS) -I. -o $(OUT) $(SRCS)

clean:
	rm -f $(OUT)
