#include <stddef.h>
#include <stdio.h>

#include "cmd.h"
#include "req.h"

static void	decode_status_resp(proto_frame_t *resp)
{
	int16_t		temp_x100;
	uint16_t	humidity_x100;
	uint8_t		fan_mode;
	uint8_t		fan_state;
	uint16_t	errors;
	int		pos;
	uint8_t		*payload;
	
	pos = 0;
	payload = resp->payload;
	temp_x100 = payload[pos++];
	temp_x100 = (temp_x100 << 8) | payload[pos++];
	humidity_x100 = payload[pos++];
	humidity_x100 = (humidity_x100 << 8) | payload[pos++];
	fan_mode = payload[pos++];
	fan_state = payload[pos++];
	errors = payload[pos++];
	errors = (errors << 8) | payload[pos];
	printf("\nStatus:");
	printf(" temperature=%.2f°C", (float)temp_x100 / 100.0f);
	printf(" humidity=%.2f%%", (float)humidity_x100 / 100.0f);
	printf(" fan_mode=%s", fan_mode == PROTO_FAN_MODE_AUTO ? "AUTO" : "MANUAL");
	printf(" fan_state=%s", fan_state == PROTO_FAN_STATE_ON ? "ON" : "OFF");
	printf(" errors=0x%04x\n\n", errors);
}

static void	decode_ack(proto_frame_t *resp)
{
	uint8_t	orig_cmd;
	uint8_t	res;

	orig_cmd = resp->payload[0];
	res = resp->payload[1];
	if (orig_cmd == PROTO_CMD_SET_FAN_MODE)
		printf("Set fan mode ");
	else if (orig_cmd == PROTO_CMD_SET_FAN_STATE)
		printf("Set fan state ");
	else if (orig_cmd == PROTO_CMD_SET_THRESHOLD)
		printf("Set threshold ");
	if (res == PROTO_ERR_OK)
		printf("OK\n");
	else if (res == PROTO_ERR_INVALID_ARG)
		printf("FAILED: invalid arg\n");
	else if (res == PROTO_ERR_STATE)
		printf("FAILED: state\n");
}

bool	do_ping(int fd)
{
	proto_frame_t	resp;

	if (!req_w8(fd, PROTO_CMD_PING, NULL, 0, &resp))
		return false;
	if (resp.cmd != PROTO_CMD_PONG)
		return false;
	printf("PONG\n");
	return true;
}

bool	do_status(int fd)
{
	proto_frame_t	resp;

	if (!req_w8(fd, PROTO_CMD_STATUS_REQ, NULL, 0, &resp))
		return false;
	if (resp.cmd != PROTO_CMD_STATUS_RESP || resp.len != sizeof(status_resp_t))
		return false;
	decode_status_resp(&resp);
	return true;
}

bool	do_set_fan_mode(int fd, uint8_t mode)
{
	proto_frame_t	resp;
	uint8_t		payload[1];

	payload[0] = mode;
	if (!req_w8(fd, PROTO_CMD_SET_FAN_MODE, payload, 1, &resp))
		return false;
	if (resp.cmd != PROTO_CMD_ACK || resp.len != 2)
		return false;
	decode_ack(&resp);
	return true;
}

bool	do_set_fan_state(int fd, uint8_t state)
{
	proto_frame_t	resp;
	uint8_t		payload[1];

	payload[0] = state;
	if (!req_w8(fd, PROTO_CMD_SET_FAN_STATE, payload, 1, &resp))
		return false;
	if (resp.cmd != PROTO_CMD_ACK || resp.len != 2)
		return false;
	decode_ack(&resp);
	return true;
}

bool	do_set_threshold(int fd, float temp)
{
	proto_frame_t	resp;
	uint8_t		payload[2];
	int16_t		temp_x100;

	temp_x100 = (int16_t)(temp * 100.0f);
	payload[0] = (uint8_t)((temp_x100 >> 8) & 0xFF);
	payload[1] = (uint8_t)(temp_x100 & 0xFF);
	if (!req_w8(fd, PROTO_CMD_SET_THRESHOLD, payload, 2, &resp))
		return false;
	if (resp.cmd != PROTO_CMD_ACK || resp.len != 2)
		return false;
	decode_ack(&resp);
	return true;
}
