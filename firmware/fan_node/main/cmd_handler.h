#pragma once

void	handle_status_req(proto_frame_t *req);
void	handle_set_fan_mode(proto_frame_t *req);
void	handle_set_fan_state(proto_frame_t *req);
void	handle_set_threshold(proto_frame_t *req);
void	handle_ping(proto_frame_t *req);
