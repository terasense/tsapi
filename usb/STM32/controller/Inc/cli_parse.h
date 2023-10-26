#pragma once

#include "errors.h"

void  cli_parser_init(void);
err_t cli_parse(const char* str, unsigned sz);

