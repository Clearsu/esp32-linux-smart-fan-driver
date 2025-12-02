#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define PROTO_SYNC0	0xAA
#define PROTO_SYNC1	0x55
#define PROTO_MAX_PAYLOAD 32

typedef struct {
	uint8_t	cmd;
	uint8_t	seq;
	uint8_t	len;
	uint8_t	payload[PROTO_MAX_PAYLOAD];
}	proto_frame_t;

uint16_t	proto_crc16(const uint8_t *data, uint16_t len);
bool		proto_build_frame(uint8_t cmd, uint8_t seq, const uint8_t *payload,
							uint8_t len, uint8_t *out, uint16_t *out_len);

typedef enum {
	RX_SYNC0,
	RX_SYNC1,
	RX_HEADER_CMD,
	RX_HEADER_SEQ,
	RX_HEADER_LEN,
	RX_PAYLOAD,
	RX_CRC_HI,
	RX_CRC_LO,
}	proto_rx_state_t;

typedef struct {
	proto_rx_state_t	st;
	uint8_t				cmd;
	uint8_t				seq;
	uint8_t				len;
	uint8_t				pos;
	uint8_t				payload[PROTO_MAX_PAYLOAD];
	uint16_t			crc;
	uint16_t			crc_recv;
}	proto_rx_t;

void	proto_rx_init(proto_rx_t *rx);
bool	proto_rx_feed(proto_rx_t *rx, uint8_t byte, proto_frame_t *out);

