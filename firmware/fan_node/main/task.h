#pragma once

void	sensor_task(void *arg);
void	fan_control_task(void *arg);
void	uart_read_task(void *arg);
void	cmd_handler_task(void *arg);

