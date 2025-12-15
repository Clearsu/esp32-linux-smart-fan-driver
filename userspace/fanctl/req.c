#include "serial.h"
#include "req.h"

#include <stdio.h>

bool	req_w8(int fd, uint8_t cmd, uint8_t *payload, uint8_t payload_len, proto_frame_t *out)
{
	uint8_t		frame_buf[256];
	uint8_t		read_buf[256];
	uint8_t		seq;
	uint16_t	frame_len;
	int		pos;
	int		bytes_read;
	proto_rx_t	rx;

	seq = 0;
	if (!proto_build_frame(cmd, seq, payload, payload_len, frame_buf, &frame_len))
	{
		fprintf(stderr, "Failed to build frame");
		return false;
	}
	printf("TX frame_len: %u\n", frame_len);
	for (int i = 0; i < frame_len; i++)
	{
		printf("%02X ", frame_buf[i]);
	}
	printf("\n");
	if (!serial_write(fd, frame_buf, frame_len))
	{
		fprintf(stderr, "Failed to write to serial");
		return false;
	}
	pos = 0;
	proto_rx_init(&rx);
	while (pos < (int)sizeof(read_buf))
	{
		bytes_read = serial_read(fd, read_buf + pos, sizeof(read_buf) - pos, 100); 
		if (bytes_read < 0)
			return false;
		if (bytes_read == 0) // timeout
			continue;
		for (int i = 0; i < bytes_read; i++)
		{
			if (proto_rx_feed(&rx, read_buf[pos + i], out))
				return true;
		}
		pos += bytes_read;
	}
	return false;
}
