#pragma once

#include <stdbool.h>

bool	do_ping(int fd);
bool	do_status(int fd);
bool	do_set_fan_mode(int fd);
bool	do_set_fan_state(int fd);
bool	do_set_threshold(int fd);
