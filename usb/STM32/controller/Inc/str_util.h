#pragma once

//
// String manipulation helpers
//

#include "util.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <ctype.h>

#define PREFIX_MATCHED(pref, buf, len) (len >= STRZ_LEN(pref) && !strncmp(pref, (const char*)buf, STRZ_LEN(pref)))

static inline bool is_space(char c)
{
	return c == ' ' || c == '\t';
}

static inline unsigned skip_spaces(const char* str, unsigned sz)
{
	unsigned skip = 0;
	while (sz && is_space(*str)) {
		++skip;
		++str;
		--sz;
	}
	return skip;
}

// Convert value to single hexadecimal digit
static inline char to_hex(uint8_t v)
{
	return v >= 0xA ? 'A' + v - 0xA : '0' + v;
}

// Convert 8 bit value to hexadecimal representation
static inline void byte_to_hex(uint8_t v, char buf[2])
{
	buf[0] = to_hex(v >> 4);
	buf[1] = to_hex(v & 0xf);
}

// Convert 16 bit value to hexadecimal representation
static inline void u16_to_hex(uint16_t v, char buf[4])
{
	byte_to_hex(v >> 8, buf);
	byte_to_hex(v, buf + 2);
}

// Convert 32 bit value to hexadecimal representation
static inline void u32_to_hex(uint32_t v, char buf[8])
{
	u16_to_hex(v >> 16, buf);
	u16_to_hex(v, buf + 4);
}

// Read unsigned value from the buffer. The value may start from the radix prefix.
// Returns the number of bytes consumed or 0 if buffer does not start with the number.
static inline unsigned scan_u(const char* buf, unsigned len, uint32_t* val)
{
	uint32_t v = 0;
	unsigned const in_len = len;
	unsigned radix = 10, dig_cnt = 0;
	for (; len; --len, ++buf)
	{
		unsigned char d;
		char const c = *buf;
		if (!dig_cnt) {
			switch (c) {
			case ' ':
			case '\t':
			case '#':
				continue;
			case 'X':
			case 'x':
			case 'H':
			case 'h':
				radix = 16;
				continue;
			case 'Q':
				radix = 8;
				continue;
			case 'B':
				radix = 2;
				continue;
			default:
				;
			}
		}
		if (c < '0')
			break;
		if (radix <= 10 && c >= '0' + radix)
			break;
		if (c <= '9')
			d = c - '0';
		else if ('a' <= c && c <= 'f')
			d = c - 'a' + 0xa;
		else if ('A' <= c && c <= 'F')
			d = c - 'A' + 0xA;
		else
			break;
		v *= radix;
		v += d;
		++dig_cnt;
	}
	if (!dig_cnt)
		return 0;
	*val = v;
	return in_len - len;
}

static inline bool has_prefix_casei(const char* str, unsigned sz, const char* pref)
{
	for (;; ++str, ++pref, --sz) {
		const char p = *pref;
		if (!p)
			return true;
		if (!sz)
			return false;
		if (tolower(*str) != tolower(p))
			return false;
	}
}
