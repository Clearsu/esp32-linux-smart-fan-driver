#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define PROTO_MAX_PAYLOAD 32

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

typedef enum {
	PROTO_SYNC0	= 0xAA,
	PROTO_SYNC1	= 0x55,
}	proto_sync_t;

typedef enum {
	PROTO_CMD_STATUS_REQ = 0x01,
	PROTO_CMD_SET_FAN_MODE = 0x02,
	PROTO_CMD_SET_FAN_STATE = 0x03,
	PROTO_CMD_SET_THRESHOLD = 0x04,
	PROTO_CMD_PING = 0x05,
	PROTO_CMD_STATUS_RESP = 0x81,
	PROTO_CMD_ACK = 0x82,
	PROTO_CMD_PONG = 0x83,
}	proto_cmd_t;

typedef enum {
	PROTO_ERR_OK = 0x00,
	PROTO_ERR_INVALID_ARG = 0x01,
	PROTO_ERR_STATE = 0x02,
}	proto_err_t;

typedef enum {
	PROTO_FAN_STATE_OFF = 0,
	PROTO_FAN_STATE_ON = 1,
}	proto_fan_state_t;

typedef enum {
	PROTO_FAN_MODE_AUTO = 0,
	PROTO_FAN_MODE_MANUAL = 1,
}	proto_fan_mode_t;

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

typedef struct {
	uint8_t	cmd;
	uint8_t	seq;
	uint8_t	len;
	uint8_t	payload[PROTO_MAX_PAYLOAD];
}	proto_frame_t;

typedef struct __attribute__((packed)) {
	int16_t		temp_x100; // 0.01°C
	uint16_t	humidity_x100; // 0.01% RH
	uint8_t		fan_mode; // 0=AUTO, 1=MANUAL
	uint8_t		fan_state; // 0=OFF, 1=ON
	uint16_t	errors; // bitfield
}	status_resp_t;

uint16_t	proto_crc16(const uint8_t *data, uint16_t len);
bool		proto_build_frame(uint8_t cmd, uint8_t seq, const uint8_t *payload,
							uint8_t len, uint8_t *out, uint16_t *out_len);
void		proto_rx_init(proto_rx_t *rx);
bool		proto_rx_feed(proto_rx_t *rx, uint8_t byte, proto_frame_t *out);

