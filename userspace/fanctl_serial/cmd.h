#pragma once

#include <stdint.h>
#include <stdbool.h>

bool	do_ping(int fd);
bool	do_status(int fd);
bool	do_set_fan_mode(int fd, uint8_t mode);
bool	do_set_fan_state(int fd, uint8_t state);
bool	do_set_threshold(int fd, float temp);
