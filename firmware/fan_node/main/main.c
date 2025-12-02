#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/uart.h"

#include "sensor.h"
#include "servo.h"
#include "communication.h"

typedef struct {
	float temperature;
	bool fan_on;
} system_state_t;

static system_state_t g_state;

void sensor_task(void *arg)
{
	float temperature;
	float humidity;

	vTaskDelay(pdMS_TO_TICKS(2000));
	dht_gpio_init();
	while (1)
	{
		if (dht_read(&temperature, &humidity))
		{
			g_state.temperature = temperature;
			ESP_LOGI("SENSOR", "temp = %.1f, humidity = %.1f", temperature, humidity);
		} else
			ESP_LOGE("SENSOR", "sensor error");
		vTaskDelay(pdMS_TO_TICKS(2100));
	}
}

void fan_control_task(void *arg)
{
	const float THRESHOLD = 28.0f;

	sg90_init();
	while (1)
	{
		if (g_state.temperature < THRESHOLD) {
			vTaskDelay(pdMS_TO_TICKS(2000));
			continue;
		}
		sg90_set_angle(0);
		vTaskDelay(pdMS_TO_TICKS(1000));
		sg90_set_angle(180);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}

	while (1)
	{
		if (g_state.temperature >= THRESHOLD)
			g_state.fan_on = true;
		else
			g_state.fan_on = false;
		//ESP_LOGI("CTRL", "temp = %.1f fan = %s", g_state.temperature, g_state.fan_on ? "ON" : "OFF");
		vTaskDelay(pdMS_TO_TICKS(2000));
	}
}

void uart_task(void *arg)
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
				{
					ESP_LOGI("UART", "RX frame: cmd=0x%02X seq=%u len=%u",
							frame.cmd, frame.seq, frame.len);
					if (frame.len > 0)
					{
						printf("payload:");
						for (int j = 0; j < frame.len; j++)
							printf(" %02X", frame.payload[j]);
						printf("\n");
					}
					// put it in a queue
					// command handler task will process the frame
				}
			}
		}
		vTaskDelay(pdMS_TO_TICKS(10));
	}
}

void app_main(void)
{
	BaseType_t ret;

	ESP_LOGI("MAIN", "app_main start");
//	ret = xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 5, NULL);
//	if (ret != pdPASS)
//		ESP_LOGE("MAIN", "Failed to create sensor_task");
//	ret = xTaskCreate(fan_control_task, "fan_control_task", 4096, NULL, 5, NULL);
//	if (ret != pdPASS)
//		ESP_LOGE("MAIN", "Failed to create fan_control_task");
	ret = xTaskCreate(uart_task, "uart_task", 4096, NULL, 4, NULL);
	if (ret != pdPASS)
		ESP_LOGE("MAIN", "Failed to create uart_task");
	ESP_LOGI("MAIN", "app_main end");

	/*
	uint8_t buf[64];
	uint16_t out_len;

	uint8_t payload[] = {0x12, 0x84};

	proto_build_frame(0x81, 0x01, payload, sizeof(payload), buf, &out_len);
	printf("TX frame (%d bytes): ", out_len);
	for (int i = 0; i < out_len; i++) {
		printf("%02X ", buf[i]);
	}
	printf("\n");
	*/


}
