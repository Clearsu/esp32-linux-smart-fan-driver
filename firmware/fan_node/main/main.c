#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "proto.h"
#include "sensor.h"

typedef struct {
	float temperature;
	bool fan_on;
} system_state_t;

static system_state_t g_state;

void sensor_task(void *arg)
{
	float t;

	t = 24.0f;
	while (1)
	{
		t += 0.5f;
		if (t > 32.0f)
		{
			t = 24.0f;
		}
		g_state.temperature = t;
		ESP_LOGI("SENSOR", "fake temp = %.1f", t);
		vTaskDelay(pdMS_TO_TICKS(2000));
	}
}

void fan_control_task(void *arg)
{
	const float THRESHOLD = 28.0f;

	while (1)
	{
		if (g_state.temperature >= THRESHOLD)
		{
			g_state.fan_on = true;
		}
		else
		{
			g_state.fan_on = false;
		}
		ESP_LOGI("CTRL", "temp = %.1f fan = %s", g_state.temperature, g_state.fan_on ? "ON" : "OFF");
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

void app_main(void)
{
	/*
	BaseType_t ret;

	ESP_LOGI("MAIN", "app_main start");
	ret = xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 5, NULL);
	if (ret != pdPASS)
	{
		ESP_LOGE("MAIN", "Failed to create sensor_task");
	}
	ret = xTaskCreate(fan_control_task, "fan_control_task", 4096, NULL, 5, NULL);
	if (ret != pdPASS)
	{
		ESP_LOGE("MAIN", "Failed to create sensor_task");
	}
	ESP_LOGI("MAIN", "app_main end");
	*/

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

	float temp = 0;
	float humidity = 0;

	esp_rom_delay_us(3000000);

	dht_gpio_init();
	dht_read(&temp, &humidity);
	printf("temp: %f, humidity: %f\n", temp, humidity);
}
