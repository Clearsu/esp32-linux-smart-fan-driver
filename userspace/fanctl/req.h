#pragma once

#include "proto.h"

bool	req_w8(int fd, uint8_t cmd, uint8_t *payload, uint8_t payload_len, proto_frame_t *out);
