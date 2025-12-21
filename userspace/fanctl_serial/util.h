#pragma once

typedef enum {
	PARSE_TEMP_ERR_OK = 0,
	PARSE_TEMP_ERR_FORMAT = 1,
	PARSE_TEMP_ERR_RANGE = 2,
} parse_temp_err_t;

float	parse_temp_str(char *temp_str, int *err);
int	get_curr_time_milli(void);
