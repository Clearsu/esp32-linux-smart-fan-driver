#include <stddef.h>
#include <stdio.h>

#include "cmd.h"
#include "req.h"

bool	do_ping(int fd)
{
	proto_frame_t	resp;

	if (!req_w8(fd, PROTO_CMD_PING, NULL, 0, &resp))
		return false;
	if (resp.cmd == PROTO_CMD_PONG)
	{
		printf("PONG");
		return true;
	}
	return false;
}

bool	do_status(int fd)
{
	return true;
}

bool	do_set_fan_mode(int fd)
{
	return true;
}

bool	do_set_fan_state(int fd)
{
	return true;
}

bool	do_set_threshold(int fd)
{
	return true;
}
