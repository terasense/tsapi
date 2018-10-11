//
// Commands handlers
//

#include "commands.h"

static u8 cmdErrors;
static u8 cmdLastSn;

void CmdInit(void)
{
	cmdErrors = 0;
	cmdLastSn = 0;
}

void CmdSetError(u8 err)
{
	cmdErrors |= err;
}

static u8 getDeviceStatus(void)
{
	return 0;
}

static void cmdInfo(ts_usb_cmd_t xdata* cmd)
{
	s8 i;
	char* bld = BUILD_STR;
	ts_usb_info_data_t* info = (ts_usb_info_data_t*)cmd->buff;
	info->vmaj = VER_MAJ;
	info->vmin = VER_MIN;
	info->last_sn = cmdLastSn;
	for (i = 0; i < fld_size(ts_usb_info_data_t, build); ++i) {
		info->build[i] = bld[i];
		if (!bld[i])
			break;
	}
}

static void cmdReset(ts_usb_cmd_t xdata* cmd)
{
	ts_usb_reset_data_t* d = (ts_usb_reset_data_t*)cmd->buff;
	// Reset command processor state
	CmdInit();
}

void CmdProcess(ts_usb_cmd_t xdata* cmd)
{
	u8 command = cmd->cmd & CMD_MASK;
	if ((cmdErrors & TS_ERR_PROTO) && command > TS_RESET) {
		cmd->s_err |= TS_ERR_PROTO;
		goto done;
	}
	if (cmd->s_err & SEQ_MODE)
	{
		s8 d;
		if (cmdErrors & TS_ERR_SEQ) {
			cmd->s_err |= TS_ERR_SEQ;
			goto done;
		}
		d = (s8)(cmd->sn - cmdLastSn);
		if (d <= 0) {
			// Already processed - ignore it
			cmd->cmd = 0;
			return;
		}
		if (d > 1) {
			// Some commands are lost
			CmdSetError(TS_ERR_SEQ);
			cmd->s_err |= TS_ERR_SEQ;
			goto done;
		}
		cmdLastSn = cmd->sn;
	}
	switch (command) {
		case TS_INFO:
			cmdInfo(cmd);
			break;
		case TS_RESET:
			cmdReset(cmd);
			break;
		default:
			CmdSetError(TS_ERR_PROTO);
			cmd->s_err |= TS_ERR_PROTO;
	}
done:
	cmd->status = cmdErrors | getDeviceStatus();
}


