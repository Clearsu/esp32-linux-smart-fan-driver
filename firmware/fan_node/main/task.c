#include "driver/uart.h"

#include "sys_state.h"
#include "sg90.h"
#include "dht22.h"
#include "proto.h"
#include "comm.h"
#include "cmd_handler.h"

void	sensor_task(void *arg)
{
	float	temperature;
	float	humidity;

	vTaskDelay(pdMS_TO_TICKS(3000));
	dht_gpio_init();
	while (1)
	{
		if (dht_read(&temperature, &humidity))
		{
			g_state.temperature = temperature;
			g_state.humidity = humidity;
		}
		else
			ESP_LOGE("SENSOR", "sensor error");
		vTaskDelay(pdMS_TO_TICKS(2100));
	}
}

void	fan_control_task(void *arg)
{
	sg90_init();
	while (1)
	{
		if (g_state.fan_mode == PROTO_FAN_MODE_AUTO)
		{
			if (g_state.temperature >= g_state.temp_threshold)
			{
				if (g_state.fan_state == PROTO_FAN_STATE_OFF)
				{
					g_state.fan_state = PROTO_FAN_STATE_ON;
					ESP_LOGI("FAN", "temperature exceeded threshold: fan ON");
				}
				sg90_set_angle(0);
				vTaskDelay(pdMS_TO_TICKS(1000));
				sg90_set_angle(180);
			} else {
				if (g_state.fan_state == PROTO_FAN_STATE_ON)
				{
					g_state.fan_state = PROTO_FAN_STATE_OFF;
					ESP_LOGI("FAN", "temperature is below threshold: fan OFF");
				}
			}
		}
		else if (g_state.fan_mode == PROTO_FAN_MODE_MANUAL)
		{
			if (g_state.fan_state == PROTO_FAN_STATE_ON)
			{
				sg90_set_angle(0);
				vTaskDelay(pdMS_TO_TICKS(1000));
				sg90_set_angle(180);
			}
		}
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

void	uart_read_task(void *arg)
{
	uint8_t			rx_buf[128];
	int				read_len;
	proto_rx_t		rx;
	proto_frame_t	frame;

	comm_init();
	proto_rx_init(&rx);
	while (1)
	{
		read_len = uart_read_bytes(COMM_UART, rx_buf, sizeof(rx_buf), pdMS_TO_TICKS(100));
		if (read_len > 0)
		{
			ESP_LOGI("UART", "RX %d bytes: ", read_len);
			for (int i = 0; i < read_len; i++)
				printf("%02X ", rx_buf[i]);
			printf("\n");
			for (int i = 0; i < read_len; i++)
			{
				if (proto_rx_feed(&rx, rx_buf[i], &frame)) {
					if (xQueueSend(g_cmd_queue, &frame, 0) != pdTRUE)
						ESP_LOGE("UART", "Queue full, drop cmd 0x%02X", frame.cmd);
				}
			}
		}
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

void	cmd_handler_task(void *arg)
{
	proto_frame_t	req;

	while (1)
	{
		if (xQueueReceive(g_cmd_queue, &req, portMAX_DELAY) == pdTRUE)
		{
			switch (req.cmd)
			{
				case PROTO_CMD_STATUS_REQ:
					ESP_LOGI("CMD", "CMD received: STATUS_REQ");
					handle_status_req(&req);
					break;
				case PROTO_CMD_SET_FAN_MODE:
					ESP_LOGI("CMD", "CMD received: SET_FAN_MODE");
					handle_set_fan_mode(&req);
					break;
				case PROTO_CMD_SET_FAN_STATE:
					ESP_LOGI("CMD", "CMD received: SET_FAN_STATE");
					handle_set_fan_state(&req);
					break;
				case PROTO_CMD_SET_THRESHOLD:
					ESP_LOGI("CMD", "CMD received: SET_THRESHOLD");
					handle_set_threshold(&req);
					break;
				case PROTO_CMD_PING:
					ESP_LOGI("CMD", "CMD received: PING");
					handle_ping(&req);
					break;
				default:
					ESP_LOGW("CMD", "Unknown CMD 0x%02X", req.cmd);
					break;
			}
		}
	}
}
