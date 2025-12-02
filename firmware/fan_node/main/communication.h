#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "protocol.h"

void	comm_init(void);
bool	comm_send_frame(const proto_frame_t *frame);
