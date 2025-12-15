CC=gcc

CFLAGS=-Wall -Wextra -O2 -Iinclude

SRCS=main.c serial.c proto.c cmd.c req.c

OUT=fanctl

all: $(OUT)

$(OUT): $(SRCS)
	$(CC) $(CFLAGS) -I. -o $@ $(SRCS)

clean:
	rm -f $(OUT)
