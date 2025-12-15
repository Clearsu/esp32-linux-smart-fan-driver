#include <string.h>
#include <stdio.h>

#include "serial.h"
#include "cmd.h"

int main(int argc, char **argv)
{
	char	*port;
	char	*cmd;
	int	fd;
	bool	res;

	if (argc < 4 || strcmp(argv[1], "-p") != 0)
	{
		printf("Usage: %s -p /dev/ttyXXX <ping|status|...>\n", argv[0]);
		return 1;
	}
	port = argv[2];
	cmd = argv[3];
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
	else if (strcmp(cmd, "set_fan_mode") == 0)
	{
		res = do_set_fan_mode(fd);
	}
	else if (strcmp(cmd, "set_fan_state") == 0)
	{
		res = do_set_fan_state(fd);
	}
	else if (strcmp(cmd, "set_threshold") == 0)
	{
		res = do_set_threshold(fd);
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
