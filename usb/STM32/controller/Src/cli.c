#include "main.h"
#include "cli.h"
#include "cli_parse.h"
#include "usb_device.h"
#include "usbd_cdc_if.h"
#include <stdbool.h>
#include <stdarg.h>

#define RX_BUFF_SZ 0x1100
#define TX_BUFF_SZ 0x1100

#define CLI_EOL     "\r"
#define CLI_EOL_CHR '\r'
#define CLI_RST_CHR '-'
#define CLI_ERR_FMT "#%04d"

/* Receive context */
static uint8_t  rx_buff[RX_BUFF_SZ+1];
static unsigned rx_sz;
static int      rx_cmd_sz;

/* Transmit context */
static uint8_t  tx_buff[TX_BUFF_SZ+1];
static unsigned tx_sz;

static err_t    cli_err;    // the error code

/* Total number of successfully processed commands */
static unsigned cmd_total;

static inline void tx_reset(void)
{
	tx_sz = 0;
}

static inline void rx_reset(void)
{
	rx_sz = 0;
	rx_cmd_sz = 0;
	cli_err = err_ok;
}

static err_t cli_reply(void)
{
	uint8_t const rc = CDC_Transmit_FS(tx_buff, tx_sz);
	tx_reset();
	return rc == USBD_OK ? err_ok : err_internal;
}

err_t cli_put(char const* buff, unsigned sz)
{
	if (tx_sz + sz > TX_BUFF_SZ)
		return err_internal;
	memcpy((char*)tx_buff + tx_sz, buff, sz);
	tx_sz += sz;
	return err_ok;
}

__PRINTFPR err_t cli_printf(const char* fmt, ...)
{
	int res;
	int space = TX_BUFF_SZ - tx_sz;
	va_list args;
	va_start(args, fmt);
	res = vsnprintf((char*)tx_buff + tx_sz, space, fmt, args);
	va_end(args);
	if (res < 0 || res > space)
		return err_internal;
	tx_sz += res;
	return err_ok;
}

static err_t cli_eol(void)
{
	if (tx_sz < TX_BUFF_SZ) {
		char* ptr = (char*)tx_buff + tx_sz;
		*ptr = *CLI_EOL;
		++tx_sz;
		return err_ok;
	} else {
		return err_internal;
	}
}

static err_t cli_respond_err(err_t res)
{
	tx_reset();
	cli_printf(CLI_ERR_FMT CLI_EOL, res);
	return cli_reply();
}

static err_t cli_handle_input(unsigned sz)
{
	err_t err;
	if (!sz)
		return err_internal;
	if (
		(err = cli_parse((const char*)rx_buff, sz - 1)) ||
		(err = cli_eol())
	)
		return err;

	return cli_reply();
}

static inline int usb_connected(void)
{
	return hUsbDeviceFS.dev_state == USBD_STATE_CONFIGURED;
}

static inline int usb_busy(void)
{
	USBD_CDC_HandleTypeDef* hcdc = (USBD_CDC_HandleTypeDef*)hUsbDeviceFS.pClassData;
	return !hcdc || hcdc->TxState != 0 || (USB_OTG_FS->GRSTCTL & USB_OTG_GRSTCTL_TXFFLSH);
}

static bool is_receive_completed(void)
{
	if (cli_err)
		return true;
	return rx_sz && rx_buff[rx_sz - 1] == CLI_EOL_CHR;
}

static bool has_reset_token(void)
{
	return	rx_sz >= 2 &&
		rx_buff[rx_sz - 1] == CLI_EOL_CHR &&
		rx_buff[rx_sz - 2] == CLI_RST_CHR;
}

static void chk_receive_completed(void)
{
	if (has_reset_token()) {
		rx_reset();
		return;
	}
	if (rx_cmd_sz) {
		// we have not processed previous message yet
		if (rx_sz)
			cli_err = err_proto;
	}
	if (is_receive_completed()) {
		if (!cli_err) {
			rx_cmd_sz = rx_sz;
			rx_buff[rx_cmd_sz] = 0;
		} else {
			rx_cmd_sz = -1;
			rx_buff[0] = 0;
		}
		rx_sz = 0;
	}
}

void cli_init(void)
{
	cli_parser_init();
}

void cli_receive(uint8_t* Buf, uint32_t *Len)
{
	unsigned len = *Len;
	if (!cli_err) {
		if (rx_sz + len > RX_BUFF_SZ) {
			cli_err = err_proto;
		} else {
			memcpy(rx_buff + rx_sz, Buf, len);
			rx_sz += len;
		}
	}
	chk_receive_completed();
}

void cli_run(void)
{
	err_t err;
	unsigned sz;
	if (!usb_connected()) {
		return;
	}
	if (usb_busy()) {
		// Don't do anything till the previous packet transmission completion
		return;
	}
	// Check we need to reply
	if (!tx_sz && (sz = rx_cmd_sz))
	{
		rx_cmd_sz = 0;
		// Process incoming command
		if (!cli_err) {
			err = cli_handle_input(sz);
		} else {
			err = cli_err;
		}
		if (err) {
			cli_err = cli_respond_err(err);
		} else {
			++cmd_total;
		}
	}
}

