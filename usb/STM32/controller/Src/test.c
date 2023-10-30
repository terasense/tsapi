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
	next_sn = 0;
}

static void test_stop(void)
{
	test_active = false;
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
	tx_buff[0] = START_TAG0;
	tx_buff[1] = START_TAG1;
	HAL_SPI_Transmit_DMA(&hSPI, (uint8_t*)tx_buff, 2);
}

static void transmit_next(uint8_t len)
{
	for (uint8_t i = 0; i < len; ++i)
		tx_buff[i] = next_sn++;
	HAL_SPI_Transmit_DMA(&hSPI, (uint8_t*)tx_buff, len);
}

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)
{
	if (!test_active) {
		// stop pending
		set_idle(true);
	} else if (start_request) {
		// restart pending
		set_idle(true);
	} else if (READ_PIN(FX_nHS)) {
		// not in HS mode
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
		return is_idle ? "STOPPED" : "STOPPING";
	else
		return is_idle ? "PAUSED" : "RUNNING";
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

const struct scpi_node test_fifo_nodes[] = {
	{
		"STATe",
		NULL,
		test_state_handler,
		" (RUN|STOP) starts|stops FX2 FIFO test. STATe? returns its current state."
	},
	SCPI_NODE_END
};
