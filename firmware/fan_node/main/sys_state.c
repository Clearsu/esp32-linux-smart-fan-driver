#include "sys_state.h"
#include "freertos/semphr.h"

static sys_state_t			g_state;
static SemaphoreHandle_t	g_state_mtx;

QueueHandle_t	g_cmd_queue;

void	sys_state_init(void)
{
	g_state_mtx = xSemaphoreCreateMutex();
	g_state.fan_state = PROTO_FAN_STATE_OFF;
	g_state.fan_mode = PROTO_FAN_MODE_AUTO;
	g_state.temp_threshold = 20.0f;
}

static inline void	lock(void)
{
	xSemaphoreTake(g_state_mtx, portMAX_DELAY);
}

static inline void	unlock(void)
{
	xSemaphoreGive(g_state_mtx);
}

void	sys_state_get_state(sys_state_t *out)
{
	lock();
	*out = g_state;
	unlock();
}

void	sys_state_set_temperature(float t)
{
	lock();
	g_state.temperature = t;
	unlock();
}

void	sys_state_set_humidity(float h)
{
	lock();
	g_state.humidity = h;
	unlock();
}

void	sys_state_set_fan_state(proto_fan_state_t state)
{
	lock();
	g_state.fan_state = state;
	unlock();
}

void	sys_state_set_fan_mode(proto_fan_mode_t mode)
{
	lock();
	g_state.fan_mode = mode;
	unlock();
}

void	sys_state_set_threshold(float t)
{
	lock();
	g_state.temp_threshold = t;
	unlock();
}

