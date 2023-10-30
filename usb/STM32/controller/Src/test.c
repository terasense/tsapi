#include "test.h"
#include "main.h"
#include "str_util.h"
#include "io_util.h"
#include "scpi.h"
#include "cli.h"

#include <stdbool.h>

#define hSPI hspi1
#define SPI_BITS 16
#define TX_BURST (128/SPI_BITS)
#define TX_CHUNK (32/SPI_BITS)

static uint16_t next_sn;
static uint16_t received_sn;
static uint16_t window_sz = 0x800; // 2k * 32 = 64kbyte
static uint16_t tx_buff[TX_BURST];

static bool test_active = false;
static bool is_idle = true;

static void set_idle(bool flag)
{
	is_idle = flag;
}

static void test_start(void)
{
	test_active = true;
}

static void test_stop(void)
{
	test_active = false;
}

void test_init(void)
{
}

static void tx_start(uint8_t len)
{
	for (uint8_t i = 0; i < len; ++i)
		tx_buff[i] = next_sn++;
	HAL_SPI_Transmit_DMA(&hSPI, (uint8_t*)tx_buff, len);
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if (READ_PIN(FX_nHS)) {
		// not in HS mode
		set_idle(true);
	} else if (READ_PIN(FX_nAFULL)) {
		// has room for the full burst
		tx_start(TX_BURST);
	} else {
		// almost full
		if (READ_PIN(FX_nFULL))
			tx_start(TX_CHUNK);
		else // full
			set_idle(true);
	}
}

void test_run(void)
{
	if (!is_idle)
		return;
	if (READ_PIN(FX_nHS))
		return;
	if (!READ_PIN(FX_nAFULL))
		return;
	set_idle(false);
	tx_start(TX_BURST);
}

static const char* test_state(void)
{
	if (!test_active)
		return "STOPPED";
	if (is_idle)
		return "PAUSED";
	return "RUNNING";
}

static int test_state_handler(const char* str, unsigned sz, struct scpi_node const* n)
{
	if (!sz)
		return -err_cmd;

	if (*str == '?') {
		err_t const err = cli_printf("%s", test_state());
		if (err)
			return -err;
		return sz;
	}

	unsigned const skip = skip_spaces(str, sz);
	sz  -= skip;
	str += skip;

	if (has_prefix_casei(str, sz, "START")) {
		test_start();
		return skip + STRZ_LEN("START");
	}
	if (has_prefix_casei(str, sz, "STOP")) {
		test_stop();
		return skip + STRZ_LEN("STOP");
	}

	return -err_cmd;
}

const struct scpi_node test_tx_nodes[] = {
	{
		"POSition",
		NULL,
		scpi_u16_r_handler,
		"? returns the current transmit stream position.",
		.param = &next_sn
	},
	{
		"WINDow",
		NULL,
		scpi_u16_rw_handler,
		" N sets transmit window to N. WINDow? returns the current value.",
		.param = &window_sz
	},
	SCPI_NODE_END
};

const struct scpi_node test_rx_nodes[] = {
	{
		"POSition",
		NULL,
		scpi_u16_w_handler,
		" N sets the stream position received by the host.",
		.param = &received_sn
	},
	SCPI_NODE_END
};

const struct scpi_node test_fifo_nodes[] = {
	{
		"STATe",
		NULL,
		test_state_handler,
		" (RUN|STOP) starts|stops FX2 FIFO test. STATe? returns its current state."
	},
	{
		"TRANsmit",
		test_tx_nodes,
	},
	{
		"RECeive",
		test_rx_nodes,
	},
	SCPI_NODE_END
};
