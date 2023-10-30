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

#define START_TAG0 0x8dbe
#define START_TAG1 0x3ad6

static uint16_t next_sn;
static uint16_t received_sn;
static uint16_t window_sz = 0x800; // 2k * 32 = 64kbyte
static uint16_t tx_buff[TX_BURST];

static bool test_active   = false;
static bool start_request = false;
static bool is_idle       = true;

static void set_idle(bool flag)
{
	is_idle = flag;
}

static void test_start(void)
{
	test_active = start_request = true;
}

static void test_stop(void)
{
	// TDB
}

void test_init(void)
{
	// The SPI clock may remain at low level even though its configured as having high idle level.
	// Sending dummy word fixes this problem. Note that both FX2 and SPI ADC should be reset at
	// this point. They should be released from reset right after this call in sys_init() routine.
	uint16_t dummy = 0;
	HAL_SPI_Transmit(&hSPI, (uint8_t*)&dummy, 1, HAL_MAX_DELAY);
}

static void transmit_start(void)
{
	next_sn = 0;
	received_sn = 0;
	tx_buff[0] = START_TAG0;
	tx_buff[1] = START_TAG1;
	HAL_SPI_Transmit_DMA(&hSPI, (uint8_t*)tx_buff, 2);
}

static inline bool has_wnd_space(uint8_t len)
{
	uint16_t const end = next_sn + len;
	return (end - received_sn) <= window_sz;
}

static void transmit_next(uint8_t len)
{
	for (uint8_t i = 0; i < len; ++i)
		tx_buff[i] = next_sn++;
	HAL_SPI_Transmit_DMA(&hSPI, (uint8_t*)tx_buff, len);
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if (start_request) {
		// restart pending
		set_idle(true);
	} else if (READ_PIN(FX_nHS)) {
		// not in HS mode
		set_idle(true);
	} else if (!has_wnd_space(TX_BURST)) {
		// receiver buffer congestion
		set_idle(true);
	} else if (READ_PIN(FX_nAFULL)) {
		// has room for the full burst
		transmit_next(TX_BURST);
	} else if (READ_PIN(FX_nFULL)) {
		// almost full
		transmit_next(TX_CHUNK);
	} else // full
		set_idle(true);
}

void test_run(void)
{
	if (!test_active)
		return;
	if (!is_idle)
		return;
	if (READ_PIN(FX_nHS))
		return;
	if (!READ_PIN(FX_nAFULL))
		return;
	if (!has_wnd_space(TX_BURST))
		return;

	set_idle(false);
	if (start_request) {
		start_request = false;
		transmit_start();
	} else
		transmit_next(TX_BURST);
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
