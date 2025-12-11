#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "sys_state.h"
#include "task.h"

void app_main(void)
{
	BaseType_t ret;

	ESP_LOGI("MAIN", "app_main start");

	g_cmd_queue = xQueueCreate(8, sizeof(proto_frame_t));
	if (!g_cmd_queue)
		ESP_LOGE("MAIN", "Failed to create cmd queue");

	ret = xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 5, NULL);
	if (ret != pdPASS)
		ESP_LOGE("MAIN", "Failed to create sensor_task");
	ret = xTaskCreate(fan_control_task, "fan_control_task", 4096, NULL, 5, NULL);
	if (ret != pdPASS)
		ESP_LOGE("MAIN", "Failed to create fan_control_task");
	ret = xTaskCreate(uart_read_task, "uart_read_task", 4096, NULL, 5, NULL);
	if (ret != pdPASS)
		ESP_LOGE("MAIN", "Failed to create uart_read_task");
	ret = xTaskCreate(cmd_handler_task, "cmd_handler_task", 4096, NULL, 5, NULL);
	if (ret != pdPASS)
		ESP_LOGE("MAIN", "Failed to create cmd_handler_task");

	ESP_LOGI("MAIN", "app_main end");
}
