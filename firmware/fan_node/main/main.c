#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

static const char *TAG = "MAIN";

static void sensor_task(void *arg)
{
	int cnt;

	cnt = 0;
	while (1)
	{
		ESP_LOGI(TAG, "tick %d", cnt++);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

void app_main(void)
{
	BaseType_t ret;

	ESP_LOGI(TAG, "app_main start");
	ret = xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 5, NULL);
	if (ret != pdPASS)
	{
		ESP_LOGE(TAG, "Failed to create sensor_task");
	}
	ESP_LOGI(TAG, "app_main end");
}
