#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#include "fanctl_uapi.h"

static int open_dev(const char *path)
{
	int	fd;
	
	fd = open(path, O_RDWR);
	if (fd < 0)
	{
		perror("open");
	}
	return fd;
}

static int do_ping(int fd)
{
	if (ioctl(fd, FANCTL_IOC_PING) < 0)
	{
		perror("ioctl(PING)");
		return -1;
	}
	printf("PONG\n");
	return 0;
}

static int do_status(int fd)
{
	struct fanctl_status	st;

	memset(&st, 0, sizeof(st));
	if (ioctl(fd, FANCTL_IOC_GET_STATUS, &st) < 0)
	{
		perror("ioctl(GET_STATUS)");
		return -1;
	}
	printf("Status:\n");
	printf("  temp      = %.2f °C\n", (float)st.temp_x100 / 100.0f);
	printf("  humid     = %.2f %%\n", (float)st.humidity_x100 / 100.0f);
	printf("  fan_mode  = %s\n", st.fan_mode == 0 ? "AUTO" : "MANUAL");
	printf("  fan_state = %s\n", st.fan_state == 1 ? "ON" : "OFF");
	printf("  errors    = 0x%04x\n", st.errors);
	return 0;
}

static int do_set_fan_mode(int fd, uint8_t mode)
{
	if (ioctl(fd, FANCTL_IOC_SET_FAN_MODE, &mode) < 0)
	{
		perror("ioctl(SET_FAN_MODE)");
		return -1;
	}
	printf("OK\n");
	return 0;
}

static int do_set_fan_state(int fd, uint8_t state)
{
	if (ioctl(fd, FANCTL_IOC_SET_FAN_STATE, &state) < 0)
	{
		perror("ioctl(SET_FAN_STATE)");
		return -1;
	}
	printf("OK\n");
	return 0;
}

static int do_set_threshold(int fd, float temp_c)
{
	int16_t x100 = (int16_t)(temp_c * 100.0f);

	if (ioctl(fd, FANCTL_IOC_SET_THRESHOLD, &x100) < 0)
	{
		perror("ioctl(SET_THRESHOLD)");
		return -1;
	}
	printf("OK\n");
	return 0;
}

int main(int argc, char **argv)
{
	const char	*cmd = argv[1];
	int		fd;
	int		rc;
	float		temp;
	char		*endp;

	if (argc < 2)
	{
		fprintf(stderr, "Usage:\n  %s ping\n  %s status\n  %s auto\n"
			"  %s manual\n  %s on\n  %s off\n  %s threshold <tempC>\n",
			argv[0], argv[0], argv[0], argv[0], argv[0], argv[0], argv[0]);
		return 1;
	}
	fd = open_dev("/dev/fanctl");
	if (fd < 0)
		return 1;
	rc = 0;
	if (!strcmp(cmd, "ping"))
	{
		rc = do_ping(fd);
	}
	else if (!strcmp(cmd, "status"))
	{
		rc = do_status(fd);
	}
	else if (!strcmp(cmd, "auto"))
	{
		rc = do_set_fan_mode(fd, 0);
	}
	else if (!strcmp(cmd, "manual"))
	{
		rc = do_set_fan_mode(fd, 1);
	}
	else if (!strcmp(cmd, "on"))
	{
		rc = do_set_fan_state(fd, 1);
	}
	else if (!strcmp(cmd, "off"))
	{
		rc = do_set_fan_state(fd, 0);
	}
	else if (!strcmp(cmd, "threshold"))
	{
		if (argc < 3)
		{
			fprintf(stderr, "need temp\n");
			rc = -1;
		}
		else
		{
			temp = strtof(argv[2], &endp);
			if (endp == argv[2] || *endp != '\0' || errno == ERANGE)
			{
				fprintf(stderr, "wrong temperature format\n");
				rc = -1;
			}
			else
				rc = do_set_threshold(fd, temp);
		}
	}
	else
	{
		fprintf(stderr, "Unknown command: %s\n", cmd);
		rc = -1;
	}
	close(fd);
	if (rc == -1)
		return 1;
	return 0;
}
