// DHT22 Temperature & Humidity Sensor

#pragma once

#include <stdbool.h>

void	dht_gpio_init(void);
bool	dht_read(float *out_temp, float *out_humid);
