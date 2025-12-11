#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "esp_log.h"

#include "sys_state.h"
#include "cmd_handler.h"
#include "comm.h"
#include "proto.h"

#define BE16(x) (((x) >> 8) | ((x) << 8))

static const char	*TAG = "CMD_HANDLER";

static void	send_ack(proto_frame_t *req, const uint8_t status)
{
	proto_frame_t	resp;

	resp.cmd = PROTO_CMD_ACK;
	resp.seq = req->seq;
	resp.payload[0] = req->cmd;
	resp.payload[1] = status;
	resp.len = 2;
	if (!comm_send_frame(&resp))
		ESP_LOGE(TAG, "Failed to send ACK");
}

void	handle_status_req(proto_frame_t *req)
{
	status_resp_t	status;
	proto_frame_t	resp;

	status.temp_x100 = BE16((int16_t)(g_state.temperature * 100.0f));
	status.humidity_x100 = BE16((uint16_t)(g_state.humidity * 100.0f));
	status.fan_mode = g_state.fan_mode;
	status.fan_state = g_state.fan_state;
	status.errors = BE16(g_state.errors);
	
	resp.cmd = PROTO_CMD_STATUS_RESP;
	resp.seq = req->seq;
	resp.len = sizeof(status);
	memcpy(resp.payload, &status, sizeof(status));
	comm_send_frame(&resp);
}

void	handle_set_fan_mode(proto_frame_t *req)
{
	proto_fan_mode_t	mode;

	if (req->len != 1)
	{
		send_ack(req, PROTO_ERR_INVALID_ARG);
		return;
	}
	mode = req->payload[0];
	if (mode != PROTO_FAN_MODE_AUTO && mode != PROTO_FAN_MODE_MANUAL)
	{
		send_ack(req, PROTO_ERR_INVALID_ARG);
		return;
	}
	g_state.fan_mode = mode;
	send_ack(req, PROTO_ERR_OK);
}

void	handle_set_fan_state(proto_frame_t *req)
{
	proto_fan_state_t	state;

	if (req->len != 1)
	{
		send_ack(req, PROTO_ERR_INVALID_ARG);
		return;
	}
	state = req->payload[0];
	if (state != PROTO_FAN_STATE_ON && state != PROTO_FAN_STATE_OFF)
	{
		send_ack(req, PROTO_ERR_INVALID_ARG);
		return;
	}
	if (g_state.fan_mode == PROTO_FAN_MODE_AUTO)
	{
		send_ack(req, PROTO_ERR_STATE);
		return;
	}
	g_state.fan_state = state;
	send_ack(req, PROTO_ERR_OK);
}

void	handle_set_threshold(proto_frame_t *req)
{
	int16_t	temp_x100;
	float	temp;

	if (req->len != 2)
	{
		send_ack(req, PROTO_ERR_INVALID_ARG);
		return;
	}
	temp_x100 = (int16_t)((req->payload[0] << 8) | req->payload[1]);
	temp = (float)temp_x100 / 100;
	if (temp > 80.0f || temp < -40.0f)
	{
		send_ack(req, PROTO_ERR_INVALID_ARG);
		return;
	}
	g_state.temp_threshold = temp;
	send_ack(req, PROTO_ERR_OK);
}

void	handle_ping(proto_frame_t *req)
{
	proto_frame_t	resp;

	resp.cmd = PROTO_CMD_PONG;
	resp.seq = req->seq;
	resp.len = 0;
	comm_send_frame(&resp);
}

