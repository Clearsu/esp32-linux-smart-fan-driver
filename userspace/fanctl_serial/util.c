#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

#include "util.h"

static char *skip_ws(char *s)
{
	while (*s && isspace((unsigned char)*s))
		s++;
	return s;
}

float	parse_temp_str(char *temp_str, int *err)
{
	char	*end = NULL;
	float 	res;
	char	*ptr;
	
	if (!temp_str || !err)
		return 0.0f;
	*err = PARSE_TEMP_ERR_FORMAT;
	ptr = temp_str;
	ptr = skip_ws(ptr);
	if (*ptr == '\0')
		return 0.0f;
	errno = 0;
	res = strtof(ptr, &end);
	if (end == ptr)
		return 0.0f;
	end = (char *)skip_ws(end);
	if (*end != '\0')
		return 0.0f;
	if (errno == ERANGE) {
		*err = PARSE_TEMP_ERR_RANGE;
		return 0.0f;
	}
	if (res > 80.0f || res < -40.0f) {
		*err = PARSE_TEMP_ERR_RANGE;
		return 0.0f;
	}
	*err = PARSE_TEMP_ERR_OK;
	return res;
}

int	get_curr_time_milli(void)
{
	struct timespec	ts;

	if (timespec_get(&ts, TIME_UTC) == 0)
		return -1; // error
	return (int64_t)ts.tv_sec * 1000 + (int64_t)ts.tv_nsec / 1000000;
}
