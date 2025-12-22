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
	{
		ESP_LOGE(TAG, "Failed to send ACK");
		return;
	}
	ESP_LOGI(TAG, "ACK sent successfully");
}

void	handle_status_req(proto_frame_t *req)
{
	sys_state_t		sys_state;
	status_resp_t	status;
	proto_frame_t	resp;

	sys_state_get_state(&sys_state);

	status.temp_x100 = BE16((int16_t)(sys_state.temperature * 100.0f));
	status.humidity_x100 = BE16((uint16_t)(sys_state.humidity * 100.0f));
	status.fan_mode = sys_state.fan_mode;
	status.fan_state = sys_state.fan_state;
	status.errors = BE16(sys_state.errors);

	ESP_LOGI(TAG, "status:");
	printf("(Note: temp_x100, humid_x100, errors are converted to big-endian\n\n");
	printf("   temp_x100: %d\n", status.temp_x100);
	printf("   humid_x100: %d\n", status.humidity_x100);
	printf("   fan_mode: %s\n", status.fan_mode == PROTO_FAN_MODE_AUTO ? "AUTO" : "MANUAL");
	printf("   fan_state: %s\n", status.fan_state == PROTO_FAN_STATE_ON ? "ON" : "OFF");
	printf("   errors: 0x%04X\n\n", status.errors);
	
	resp.cmd = PROTO_CMD_STATUS_RESP;
	resp.seq = req->seq;
	resp.len = sizeof(status);
	memcpy(resp.payload, &status, sizeof(status));
	if (!comm_send_frame(&resp))
	{
		ESP_LOGE(TAG, "failed to send STATUS_RESP");
		return;
	}
	ESP_LOGI(TAG, "STATUS_RESP sent successfully");
}

void	handle_set_fan_mode(proto_frame_t *req)
{
	proto_fan_mode_t	mode;

	if (req->len != 1)
	{
		ESP_LOGW(TAG, "LEN field for SET_FAN_MODE must be 1");
		send_ack(req, PROTO_ERR_INVALID_ARG);
		return;
	}
	mode = req->payload[0];
	if (mode != PROTO_FAN_MODE_AUTO && mode != PROTO_FAN_MODE_MANUAL)
	{
		ESP_LOGW(TAG, "unkown fan mode");
		send_ack(req, PROTO_ERR_INVALID_ARG);
		return;
	}
	sys_state_set_fan_mode(mode);
	ESP_LOGI(TAG, "set fan mode to %s", mode == PROTO_FAN_MODE_AUTO ? "AUTO" : "MANUAL");
	send_ack(req, PROTO_ERR_OK);
}

void	handle_set_fan_state(proto_frame_t *req)
{
	proto_fan_state_t	fan_state;
	sys_state_t			sys_state;

	if (req->len != 1)
	{
		ESP_LOGW(TAG, "LEN field for SET_FAN_STATE must be 1");
		send_ack(req, PROTO_ERR_INVALID_ARG);
		return;
	}
	fan_state = req->payload[0];
	if (fan_state != PROTO_FAN_STATE_ON && fan_state != PROTO_FAN_STATE_OFF)
	{
		ESP_LOGW(TAG, "unkown fan state");
		send_ack(req, PROTO_ERR_INVALID_ARG);
		return;
	}
	sys_state_get_state(&sys_state);
	if (sys_state.fan_mode == PROTO_FAN_MODE_AUTO)
	{
		ESP_LOGW(TAG, "cannot change fan state: Current fan mode must be MANUAL");
		send_ack(req, PROTO_ERR_STATE);
		return;
	}
	sys_state_set_fan_state(fan_state);
	ESP_LOGI(TAG, "set fan state to %s", fan_state == PROTO_FAN_STATE_ON ? "ON" : "OFF");
	send_ack(req, PROTO_ERR_OK);
}

void	handle_set_threshold(proto_frame_t *req)
{
	int16_t	temp_x100;
	float	temp;

	if (req->len != 2)
	{
		ESP_LOGW(TAG, "LEN field for SET_THRESHOLD must be 2");
		send_ack(req, PROTO_ERR_INVALID_ARG);
		return;
	}
	temp_x100 = (int16_t)((req->payload[0] << 8) | req->payload[1]);
	temp = (float)temp_x100 / 100;
	if (temp > 80.0f || temp < -40.0f)
	{
		ESP_LOGW(TAG, "available threshold: <= 80°C and >= -40°C (request: %f)", temp);
		send_ack(req, PROTO_ERR_INVALID_ARG);
		return;
	}
	sys_state_set_threshold(temp);
	ESP_LOGI(TAG, "set threshold to %.2f", temp);
	send_ack(req, PROTO_ERR_OK);
}

void	handle_ping(proto_frame_t *req)
{
	proto_frame_t	resp;

	resp.cmd = PROTO_CMD_PONG;
	resp.seq = req->seq;
	resp.len = 0;
	if (!comm_send_frame(&resp))
	{
		ESP_LOGE(TAG, "failed to send PONG");
		return;
	}
	ESP_LOGI(TAG, "Pong sent successfully");
}

