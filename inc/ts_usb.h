#ifndef TS_USB
#define TS_USB

#include <ts_types.h>

//
// USB command packet
//

#define TS_USB_PKT_LEN 64
#define TS_USB_CMD_BUF_SZ 60

typedef struct ts_usb_cmd {
	u8 cmd;
	u8 sn;
	u8 s_err;
	u8 status;
	u8 buff[TS_USB_CMD_BUF_SZ];
} ts_usb_cmd_t;

BUILD_BUG_ON(sizeof(ts_usb_cmd_t) != TS_USB_PKT_LEN);

// High bit of cmd field marks response packet
// If set in the request the response will be sent back
// upon processing completion
#define CMD_RESP 0x80 
#define CMD_MASK 0x7f

// Sequential mode flag in the s_err field
#define SEQ_MODE 0x80
#define ERR_MASK 0x7f

//
// Command codes
//
#define TS_INFO   0 // Returns device info in response
#define TS_RESET  1 // Reset hardware and optionally reconnect USB

#define TS_EEPROM_RD 0x10
#define TS_EEPROM_WR 0x11

//
// Error code
//
#define TS_ERR_PROTO 1
#define TS_ERR_SEQ   2

#define TS_ERR_ANY (TS_ERR_PROTO|TS_ERR_SEQ)
//
// Commands data
//
typedef struct ts_usb_info_data {
	u8 vmaj;
	u8 vmin;
	u8 last_sn;
	u8 reserved[33];
	u8 build[24];
} ts_usb_info_data_t;

BUILD_BUG_ON(sizeof(ts_usb_info_data_t) != TS_USB_CMD_BUF_SZ);

#define TS_RST_FPGA 0x10

typedef struct ts_usb_reset_data {
	// With zero flags it reset error status and last sequence number
	u8 flags;
} ts_usb_reset_data_t;

typedef struct ts_usb_eeprom_data {
	u8  subaddr;
	u8  size;
	u16 offset;
	u8  buff[56];
} ts_usb_eeprom_data_t;

BUILD_BUG_ON(sizeof(ts_usb_eeprom_data_t) != TS_USB_CMD_BUF_SZ);

#endif

