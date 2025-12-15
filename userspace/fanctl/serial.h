#pragma once

#include <stdint.h>
#include <stdbool.h>

int	serial_open(const char *dev, int baud);
bool	serial_write(int fd, const uint8_t *buf, uint16_t len);
int	serial_read(int fd, uint8_t *buf, int cap, int timeout_ms);
