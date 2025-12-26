/*
 * fanctl_serial (legacy)
 * ----------------------
 * This userspace application directly communicates with the ESP32
 * over a raw serial device.
 */


#include <string.h>
#include <stdio.h>

#include "proto.h"
#include "serial.h"
#include "cmd.h"
#include "util.h"

int main(int argc, char **argv)
{
	char	*port;
	char	*cmd;
	int	fd;
	bool	res;

	if (argc < 3)
	{
		printf("Usage: %s /dev/ttyXXX <ping|status|auto|manual|on|off|threshold <tempC>>\n", argv[0]);
		return 1;
	}
	port = argv[1];
	cmd = argv[2];
	fd = serial_open(port, 115200);
	if (fd < 0)
	{
		fprintf(stderr, "Failed to open %s\n", port);
		return 1;
	}
	if (strcmp(cmd, "ping") == 0)
	{
		res = do_ping(fd);
	}
	else if (strcmp(cmd, "status") == 0)
	{
		res = do_status(fd);
	}
	else if (strcmp(cmd, "auto") == 0)
	{
		res = do_set_fan_mode(fd, PROTO_FAN_MODE_AUTO);
	}
	else if (strcmp(cmd, "manual") == 0)
	{
		res = do_set_fan_mode(fd, PROTO_FAN_MODE_MANUAL);
	}
	else if (strcmp(cmd, "on") == 0)
	{
		res = do_set_fan_state(fd, PROTO_FAN_STATE_ON);
	}
	else if (strcmp(cmd, "off") == 0)
	{
		res = do_set_fan_state(fd, PROTO_FAN_STATE_OFF);
	}
	else if (strcmp(cmd, "threshold") == 0)
	{
		float	temp;
		int	err;

		if (argc != 4)
		{
			fprintf(stderr, "Wrong number of arguments\n");
			return 1;
		}
		temp = parse_temp_str(argv[3], &err);
		if (err == PARSE_TEMP_ERR_FORMAT)
		{
			fprintf(stderr, "Wrong number format\n");
			return 1;
		}
		if (err == PARSE_TEMP_ERR_RANGE)
		{
			fprintf(stderr, "Available threshold: -40°C <= temp <= 80°C\n");
			return 1;
		}
		res = do_set_threshold(fd, temp);
	}
	else
	{
		fprintf(stderr, "Unknown command: %s\n", cmd);
		return 1;
	}
	if (!res)
	{
		fprintf(stderr, "Failed to execute %s\n", cmd);
		return 1;
	}
	return 0;
}
