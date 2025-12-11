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

	vTaskDelay(pdMS_TO_TICKS(2000));
	dht_gpio_init();
	while (1)
	{
		if (dht_read(&temperature, &humidity))
		{
			g_state.temperature = temperature;
			g_state.humidity = humidity;
			ESP_LOGI("SENSOR", "temp = %.1f, humidity = %.1f", temperature, humidity);
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
				sg90_set_angle(0);
				vTaskDelay(pdMS_TO_TICKS(1000));
				sg90_set_angle(180);
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
		read_len = uart_read_bytes(UART_NUM_0, rx_buf, sizeof(rx_buf), pdMS_TO_TICKS(100));
		if (read_len > 0)
		{
			for (int i = 0; i < read_len; i++)
			{
				if (proto_rx_feed(&rx, rx_buf[i], &frame))
					xQueueSend(g_cmd_queue, &frame, 0);
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
					handle_status_req(&req);
					break;
				case PROTO_CMD_SET_FAN_MODE:
					handle_set_fan_mode(&req);
					break;
				case PROTO_CMD_SET_FAN_STATE:
					handle_set_fan_state(&req);
					break;
				case PROTO_CMD_SET_THRESHOLD:
					handle_set_threshold(&req);
					break;
				case PROTO_CMD_PING:
					handle_ping(&req);
					break;
				default:
					ESP_LOGW("CMD", "Unknown CMD 0x%02X", req.cmd);
					break;
			}
		}
	}
}
