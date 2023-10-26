#pragma once

typedef enum {
	err_ok          = 0, /* No error */
	/*
	 * Software errors in the order of increasing severity.
	 */
	err_state       = 1, /* Improper state to execute command */
	err_param       = 2, /* Invalid command parameter */
	err_cmd         = 3, /* Invalid command */
	err_proto       = 4, /* Protocol error */
	err_internal    = 5, /* Unexpected software error */
	err_timeout     = 6, /* Timeout waiting paired device response */
} err_t;
