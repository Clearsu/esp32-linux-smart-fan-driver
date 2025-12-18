#pragma once

#ifdef __KERNEL__

#include <linux/types.h>
#include <linux/kernel.h>
typedef u8   proto_u8;
typedef u16  proto_u16;
typedef s16  proto_s16;

#else

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
typedef uint8_t  proto_u8;
typedef uint16_t proto_u16;
typedef int16_t  proto_s16;

#endif

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
	proto_u8		cmd;
	proto_u8		seq;
	proto_u8		len;
	proto_u8		pos;
	proto_u8		payload[PROTO_MAX_PAYLOAD];
	proto_u16		crc;
	proto_u16		crc_recv;
}	proto_rx_t;

typedef struct {
	proto_u8	cmd;
	proto_u8	seq;
	proto_u8	len;
	proto_u8	payload[PROTO_MAX_PAYLOAD];
}	proto_frame_t;

typedef struct __attribute__((packed)) {
	proto_s16	temp_x100; // 0.01°C
	proto_u16	humidity_x100; // 0.01% RH
	proto_u8	fan_mode; // 0=AUTO, 1=MANUAL
	proto_u8	fan_state; // 0=OFF, 1=ON
	proto_u16	errors; // bitfield
}	status_resp_t;

uint16_t	proto_crc16(const proto_u8 *data, proto_u16 len);
bool		proto_build_frame(proto_u8 cmd, proto_u8 seq, const proto_u8 *payload,
							proto_u8 len, proto_u8 *out, proto_u16 *out_len);
void		proto_rx_init(proto_rx_t *rx);
bool		proto_rx_feed(proto_rx_t *rx, proto_u8 byte, proto_frame_t *out);

