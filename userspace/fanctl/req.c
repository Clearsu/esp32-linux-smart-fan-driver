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
#ifdef DEBUG
	printf("TX frame: ");
	for (int i = 0; i < frame_len; i++)
	{
		printf("%02X ", frame_buf[i]);
	}
	printf("\n");
#endif
	if (!serial_write(fd, frame_buf, frame_len))
	{
		fprintf(stderr, "Failed to write to serial");
		return false;
	}
	pos = 0;
	proto_rx_init(&rx);
#ifdef DEBUG
	printf("RX frame: ");
#endif
	while (pos < (int)sizeof(read_buf))
	{
		bytes_read = serial_read(fd, read_buf + pos, sizeof(read_buf) - pos, 100); 
		if (bytes_read < 0)
			return false;
		if (bytes_read == 0) // timeout
			continue;
		for (int i = 0; i < bytes_read; i++)
		{
#ifdef DEBUG
			printf("%02X ", read_buf[pos + i]);
#endif
			if (proto_rx_feed(&rx, read_buf[pos + i], out))
			{
#ifdef DEBUG
				printf("\n");
#endif
				return true;
			}
		}
		pos += bytes_read;
	}
#ifdef DEBUG
	printf("\n");
#endif
	return false;
}
