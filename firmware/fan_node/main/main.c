#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

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
}
