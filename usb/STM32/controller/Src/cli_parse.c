#include "cli.h"
#include "cli_parse.h"
#include "i2c_eeprom.h"
#include "system.h"
#include "scpi.h"
#include "uuid.h"
#include "debug.h"
#include "str_util.h"
#include "io_util.h"
#include "version.h"
#include "test.h"
#include "main.h"

#include <stddef.h>
#include <stdbool.h>

#define IDN_VERSION xstr(VERSION_MAJ) "." xstr(VERSION_MIN)
#define SN_PLACEHOLDER "****************"

BUILD_BUG_ON(STRZ_LEN(SN_PLACEHOLDER) != 16);

static char idn_buff[] = VENDOR "," FAMILY "," SN_PLACEHOLDER "," IDN_VERSION;

#define SN_OFFSET STRZ_LEN(VENDOR "," FAMILY ",")

static void cli_init_idn(void)
{
	u32_to_hex(UUID[0] + UUID[2], idn_buff + SN_OFFSET);
	u32_to_hex(UUID[1], idn_buff + SN_OFFSET + 8);
}

void cli_parser_init(void)
{
	cli_init_idn();
}

static int idn_handler(const char* str, unsigned sz, struct scpi_node const* n)
{
	if (sz != 1 || *str != '?')
		return -err_cmd;
	err_t err = cli_printf("%s", idn_buff);
	if (err)
		return -err;
	return sz;
}

static int version_handler(const char* str, unsigned sz, struct scpi_node const* n)
{
	if (sz != 1 || *str != '?')
		return -err_cmd;
	err_t err = cli_put(VERSION_INFO, STRZ_LEN(VERSION_INFO));
	if (err)
		return -err;
	return sz;
}

static int echo_handler(const char* str, unsigned sz, struct scpi_node const* n)
{
	err_t err = cli_put(str, sz);
	if (err)
		return -err;
	return sz;
}

static int bootloader_handler(const char* str, unsigned sz, struct scpi_node const* n)
{
	_sys_schedule_bootloader();
	return 0;
}

static int reset_handler(const char* str, unsigned sz, struct scpi_node const* n)
{
	_sys_reset();
	return 0;
}

static bool fx2_reset_get(void)
{
	return !READ_PIN(FX_nRST);
}

static void fx2_reset_set(bool v)
{
	WRITE_PIN(FX_nRST, !v);
}

static const struct scpi_node star_nodes[] = {
	{
		"IDN",
		NULL,
		idn_handler,
		"? returns device identification string"
	},
	SCPI_NODE_END
};

static const struct scpi_node reset_nodes[] = {
	{
		"LOADer",
		NULL,
		bootloader_handler,
		" ensures the boot loader will be launched after reset"
	},
	SCPI_NODE_END
};

static const struct scpi_node fx2_nodes[] = {
	{
		"RESet",
		NULL,
		scpi_bool_rw_handler2,
		"(0|1) clear|assert FX2 reset input. RESet? returns its current state.",
		.param  = (void*)fx2_reset_get,
		.param2 = (void*)fx2_reset_set
	},
	{
		"EEPRom",
		i2c_eeprom_nodes,
		NULL,
		" provides access to EEPROM attached to FX2 by means of the following tags:"
	},
	SCPI_NODE_END
};

static const struct scpi_node system_nodes[] = {
	{
		"VERSion",
		NULL,
		version_handler,
		"? returns the controller version information"
	},
	{
		"RESet",
		reset_nodes,
		reset_handler,
		" performs controller reset. The following tag is optional:"
	},
	{
		"FX2",
		fx2_nodes,
		NULL,
		" provides access to the Cypress FX2 USB Peripheral Controller by means of the following tags:"
	},
	SCPI_NODE_END
};

static const struct scpi_node test_nodes[] = {
	{
		"FIFO",
		test_fifo_nodes,
	},
	{
		"ECHO",
		NULL,
		echo_handler,
		" echos back all characters that follows it"
	},
	SCPI_NODE_END
};

static const struct scpi_node colon_nodes[] = {
	{
		"SYSTem",
		system_nodes
	},
	{
		"TEST",
		test_nodes
	},
	SCPI_NODE_END,
};

static struct scpi_tree parse_tree = {
	star_nodes,
	colon_nodes
};

err_t cli_parse(const char* str, unsigned sz)
{
	return scpi_parse(str, sz, &parse_tree);
}

