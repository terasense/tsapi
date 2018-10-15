#ifndef COMMANDS_H
#define COMMANDS_H

#include <ts_usb.h>

#define VER_MAJ 1
#define VER_MIN 0
#define BUILD_STR __DATE__ " " __TIME__

BUILD_BUG_ON(sizeof(BUILD_STR) > fld_size(ts_usb_info_data_t, build));

void CmdInit(void);

void CmdSetError(u8 err);

void CmdProcess(ts_usb_cmd_t xdata* cmd);

#endif
