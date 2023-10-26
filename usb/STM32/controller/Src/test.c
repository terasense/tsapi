#include "test.h"
#include "main.h"
#include "util.h"

#include <stdbool.h>

#define hSPI hspi1
#define SPI_BITS 16
#define TX_BURST (128/SPI_BITS)
#define TX_CHUNK (32/SPI_BITS)

static uint16_t next_sn;
static uint16_t tx_buff[TX_BURST];

static bool is_idle = true;

static void set_idle(bool flag)
{
	is_idle = flag;
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
	if (READ_PIN(nHS)) {
		// not in HS mode
		set_idle(true);
	} else if (READ_PIN(nAFULL)) {
		// has room for the full burst
		tx_start(TX_BURST);
	} else {
		// almost full
		if (READ_PIN(nFULL))
			tx_start(TX_CHUNK);
		else // full
			set_idle(true);
	}
}

void test_run(void)
{
	if (!is_idle)
		return;
	if (READ_PIN(nHS))
		return;
	if (!READ_PIN(nAFULL))
		return;
	set_idle(false);
	tx_start(TX_BURST);
}
