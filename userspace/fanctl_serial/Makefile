CC		= gcc
CFLAGS		= -Wall -Wextra -O2
DBGFLAGS	= -DDEBUG -g

INCS		= . ../../common/
INCLUDES	= $(addprefix -I,$(INCS))

SRCS = main.c \
       serial.c \
       proto.c \
       cmd.c \
       req.c \
       util.c

OUT = fanctl

.PHONY: all debug clean

all: $(OUT)

$(OUT): $(SRCS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(SRCS)

debug:
	$(CC) $(CFLAGS) $(DBGFLAGS) $(INCLUDES) -o $(OUT) $(SRCS)

clean:
	rm -f $(OUT)
