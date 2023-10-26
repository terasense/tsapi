#pragma once

#include <stdint.h>
#include <stdio.h>
#include "errors.h"

void cli_init(void);
void cli_receive(uint8_t* Buf, uint32_t *Len);
void cli_run(void);

err_t cli_put(char const* buff, unsigned sz);

__PRINTFPR err_t cli_printf(const char* fmt, ...);

