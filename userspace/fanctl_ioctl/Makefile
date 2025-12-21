CC=gcc
CFLAGS=-Wall -Wextra -Werror

INCS		= . ../../common/
INCLUDES	= $(addprefix -I,$(INCS))

OUT=fanctl
SRCS=main.c

all: $(OUT)

$(OUT): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(INCLUDES)

clean:
	rm -f $(OUT)

.PHONY: all clean
