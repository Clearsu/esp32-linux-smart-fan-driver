#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/portmacro.h"

#include "proto.h"

typedef struct {
	float				temperature;
	float				humidity;
	proto_fan_state_t	fan_state;
	proto_fan_mode_t	fan_mode;
	float				temp_threshold;
	uint16_t			errors;
}	sys_state_t;

void	sys_state_init(void);

void	sys_state_get_state(sys_state_t *out);

void	sys_state_set_temperature(float t);
void	sys_state_set_humidity(float h);
void	sys_state_set_fan_state(proto_fan_state_t state);
void	sys_state_set_fan_mode(proto_fan_mode_t mode);
void	sys_state_set_threshold(float t);

extern QueueHandle_t	g_cmd_queue;
