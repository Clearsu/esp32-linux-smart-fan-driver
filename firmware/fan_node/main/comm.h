#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "driver/uart.h"

#include "proto.h"

#define COMM_UART UART_NUM_1
#define COMM_TX_PIN 16
#define COMM_RX_PIN 17

void	comm_init(void);
bool	comm_send_frame(const proto_frame_t *frame);
