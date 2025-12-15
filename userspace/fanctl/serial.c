#include "serial.h"

#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <poll.h>
#include <errno.h>
#include <stdio.h>

static speed_t	baud_rate(int baud)
{
	switch (baud)
	{
		case 115200:
			return B115200;
		default:
			return B115200;
	}
}

int	serial_open(const char *dev, int baud)
{
	int		fd;
	struct termios	tio;
	speed_t		sp;
	
	fd = open(dev, O_RDWR | O_NOCTTY | O_NONBLOCK);
	if (fd < 0)
	{
		perror("open");
	        return -1;
	}
	tcgetattr(fd, &tio);
	cfmakeraw(&tio);
	tio.c_cflag |= (CLOCAL | CREAD);
	tio.c_cflag &= ~CSTOPB; // 1 stop
	tio.c_cflag &= ~PARENB; // no parity
	tio.c_cflag &= ~CRTSCTS; // no hw flow
	tio.c_cflag &= ~CSIZE;
	tio.c_cflag |= CS8; // 8 bits
	sp = baud_rate(baud);
	cfsetispeed(&tio, sp);
	cfsetospeed(&tio, sp);
	tcsetattr(fd, TCSANOW, &tio);
	tcflush(fd, TCIOFLUSH);
	return fd;
}

bool	serial_write(int fd, const uint8_t *buf, uint16_t len)
{
	uint16_t	offset;
	ssize_t		n;

	offset = 0;
	while (offset < len)
	{
		n = write(fd, buf + offset, len - offset);
		if (n <= 0)
		{
			perror("open");
			return false;
		}
		offset += (uint16_t)n;
	}
	return true;
}

int	serial_read(int fd, uint8_t *buf, int cap, int timeout_ms)
{
	struct pollfd	pfd;
	int		pr;
	int		n;

	pfd.fd = fd;
	pfd.events = POLLIN;
	pr = poll(&pfd, 1, timeout_ms);
	if (pr < 0)
	{
		if (errno == EINTR)
			return 0;
		perror("poll");
		return -1;
	}
	if (pr == 0) {
		return 0; // timeout
	}
	if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))
	{
		fprintf(stderr, "poll: revents=0x%x\n", pfd.revents);
		return -1;
	}
	n = read(fd, buf, cap);
	if (n < 0) {
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return 0;
		perror("read");
		return -1;
	}
	if (n == 0) // EOF
	{
		fprintf(stderr, "read: EOF (peer closed)\n");
		return -1;
	}
	return n;
}
