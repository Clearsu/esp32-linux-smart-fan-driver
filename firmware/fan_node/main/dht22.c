/**
 *
 * Temperature and humidity sensor AM2302 (DHT22) firmware for ESP32
 * 
 * Implementation is based on the datasheet:
 * https://cdn-shop.adafruit.com/datasheets/Digital+humidity+and+temperature+sensor+AM2302.pdf
 *
 */



#include <stdio.h>

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "dht22.h"

#define DHT_GPIO 23
#define GPIO_LOW 0
#define GPIO_HIGH 1

static const char *TAG = "DHT22";

static void	dht_set_idle(void)
{
	gpio_set_level(DHT_GPIO, GPIO_HIGH);
}

static void	dht_send_start_signal(void)
{
	gpio_set_direction(DHT_GPIO, GPIO_MODE_OUTPUT);
	gpio_set_level(DHT_GPIO, GPIO_LOW);
	esp_rom_delay_us(2000);
	gpio_set_level(DHT_GPIO, GPIO_HIGH);
	esp_rom_delay_us(30);
	gpio_set_direction(DHT_GPIO, GPIO_MODE_INPUT);
}

static int16_t	dht_gpio_wait_level(int level, uint16_t timeout_us)
{
	uint16_t	timeout;
	int			ret;

	timeout = timeout_us;
	while (timeout-- > 0)
	{
		if (gpio_get_level(DHT_GPIO) == level) {
			return timeout_us - timeout;
		}
		esp_rom_delay_us(1);
	}
	return -1;
}

static bool	dht_read_start_response(void)
{
	if (dht_gpio_wait_level(GPIO_LOW, 80) < 0)
		return false;
	if (dht_gpio_wait_level(GPIO_HIGH, 80) < 0)
		return false;
	return true;
}

static int	dht_read_1bit(void)
{
	int16_t	cnt = 0;

	if (dht_gpio_wait_level(GPIO_LOW, 100) == -1 || dht_gpio_wait_level(GPIO_HIGH, 100) == -1)
		return -1;
	while (gpio_get_level(DHT_GPIO) == GPIO_HIGH && cnt < 100)
	{
		cnt++;
		esp_rom_delay_us(1);
	}
	if (cnt > 30)
		return 1;
	return 0;
}

static bool	dht_read_raw(uint8_t out[5])
{
	int	byte_index;
	int bit_index;
	int8_t buffer[40];

	memset(out, 0, 5);
	for (int i = 0; i < 40; i++)
		buffer[i] = dht_read_1bit();
	for (int i = 0; i < 40; i++) {
		if (buffer[i] == -1)
			return false;
		byte_index = i / 8;
		bit_index = 7 - (i % 8);
		out[byte_index] |= (buffer[i] << bit_index);
	}
	return true;
}

static bool dht_check_checksum(uint8_t out[5])
{
	return ((out[0] + out[1] + out[2] + out[3]) & 0xFF) == out[4];
}

void	dht_gpio_init(void)
{
	gpio_config_t cfg = {
		.pin_bit_mask = 1ULL << DHT_GPIO,
		.mode = GPIO_MODE_OUTPUT,
		.pull_up_en = GPIO_PULLUP_ENABLE,
		.pull_down_en = GPIO_PULLDOWN_DISABLE,
		.intr_type = GPIO_INTR_DISABLE,
	};
	gpio_config(&cfg);
	dht_set_idle();
}

bool	dht_read(float *out_temp, float *out_humid)
{
	uint8_t		out[5];

	dht_send_start_signal();
	if (!dht_read_start_response()) {
		ESP_LOGE(TAG, "Failed to perform start handshake");
		return false;
	}
	if (!dht_read_raw(out))
		return false;
	/*
	printf("raw out:");
	for (int i = 0; i < 5; i++)
		printf("%02X ", out[i]);
	printf("\n");
	*/
	if (!dht_check_checksum(out))
		return false;
	*out_humid = (float)((out[0] << 8) | out[1]) / 10;
	*out_temp = (float)((out[2] << 8) | out[3]) / 10;
	return true;
}
