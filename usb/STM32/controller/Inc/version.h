#pragma once

//
// Product version information
//

#include "util.h"
#include "debug.h"

#define VENDOR "TeraSense"
#define FAMILY "SmartVision"
#define VERSION_MAJ 0
#define VERSION_MIN 1
#define REVISION 1

#define VERSION_INFO FAMILY " v." xstr(VERSION_MAJ) "." xstr(VERSION_MIN) " r." xstr(REVISION) " [" __DATE__ "]"

#define FW_TAG_OFFSET 0x1000
#define FW_TAG_ADDR (FLASH_BASE + FW_TAG_OFFSET)

struct version_tag {
	uint8_t     ver_maj;
	uint8_t     ver_min;
	uint8_t     revision;
	uint8_t     flags;
	const char  info[60];
};

BUILD_BUG_ON(sizeof(struct version_tag) != 64);
BUILD_BUG_ON(sizeof(VERSION_INFO) >= 60);

#define VERSION_TAG_INI {   \
	VERSION_MAJ,        \
	VERSION_MIN,        \
	REVISION,           \
	0,                  \
	VERSION_INFO        \
}
