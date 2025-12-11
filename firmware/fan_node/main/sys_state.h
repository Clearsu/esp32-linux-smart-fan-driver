#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "proto.h"

typedef struct {
	float				temperature;
	float				humidity;
	proto_fan_state_t	fan_state;
	proto_fan_mode_t	fan_mode;
	float				temp_threshold;
	uint16_t			errors;
}	sys_state_t;

extern sys_state_t		g_state;
extern QueueHandle_t	g_cmd_queue;
