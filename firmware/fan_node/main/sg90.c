/**
 * Firmware implementation for SG90 Servo
 *
 * ##### SG90 180° Servo: Angle → PWM Duty Conversion #####
 *
 * SG90 uses PWM pulse width to represent the target angle.
 * The PWM frequency is fixed at 50Hz (period = 20 ms), and the HIGH pulse width
 * determines the servo position:
 *
 *   - 0°   ≈ 1000 µs
 *   - 90°  ≈ 1500 µs
 *   - 180° ≈ 2000 µs
 *
 * On the ESP32, the LEDC peripheral represents PWM duty as an integer value rather
 * than a percentage. The duty range depends on the timer resolution. For example,
 * with a 10-bit resolution, the duty range is 0–1023.
 *
 * The conversion process is:
 *
 *   1) Convert angle (0–180°) to pulse width (µs):
 *
 *        pulse_us = min_us + (angle_deg / 180.0) * (max_us - min_us)
 *
 *   2) Convert pulse width to duty value:
 *
 *        duty = (pulse_us / PWM_PERIOD_US) * ((1 << duty_resolution) - 1)
 *
 * Example:
 *   PWM period       = 20000 µs (50 Hz)
 *   resolution       = 10 bits (0–1023)
 *   pulse_us (90°)   = 1500 µs
 *
 *   duty = 1500 / 20000 * 1023 ≈ 76
 *
 * Writing this duty value using LEDC APIs causes the ESP32 to generate the correct
 * PWM waveform, and the servo moves to the corresponding angle.
 */

#include "driver/ledc.h"

#include "sg90.h"

#define SG90_GPIO 13
#define MAX_US 2000
#define MIN_US 1000
#define PWM_PERIOD_US 20000

static void	pwm_timer_config(void)
{
	ledc_timer_config_t timer = {
		.speed_mode = LEDC_LOW_SPEED_MODE,
		.timer_num = LEDC_TIMER_0,
		.duty_resolution = LEDC_TIMER_10_BIT,
		.freq_hz = 50,
		.clk_cfg = LEDC_AUTO_CLK,
	};
	ledc_timer_config(&timer);
}

static void	pwm_gpio_bind(void)
{
	ledc_channel_config_t channel = {
		.gpio_num = SG90_GPIO,
		.speed_mode = LEDC_LOW_SPEED_MODE,
		.channel = LEDC_CHANNEL_0,
		.timer_sel = LEDC_TIMER_0,
		.duty = 0,
		.hpoint = 0,
	};
	ledc_channel_config(&channel);
}

void	sg90_init(void)
{
	pwm_timer_config();
	pwm_gpio_bind();
}

void	sg90_set_angle(uint16_t degree)
{
	uint32_t pulse_us;
	uint32_t duty;

	pulse_us = MIN_US + (degree / 180.0) * (MAX_US - MIN_US);
	duty = (pulse_us * 1023) / PWM_PERIOD_US; // 1023 comes from 2^resolution - 1
	ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
	ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
}
