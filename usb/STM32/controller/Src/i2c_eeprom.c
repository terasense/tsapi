#include "i2c_eeprom.h"
#include "main.h"
#include "scpi.h"
#include "cli.h"
#include "errors.h"
#include "debug.h"
#include "str_util.h"
#include <stdbool.h>

#define EPM_ADDRESS    0xa2
#define EPM_ADDR_END   0x8000 // 32k for AT24C256
#define EPM_PAGE_SZ    64
#define EPM_ADDR_TOUT  2
#define EPM_DATA_TOUT  20
#define EPM_WRITE_WAIT 12 // msec

#define hEPM_I2C hi2c1

static uint8_t  i2c_epm_buff[EPM_PAGE_SZ+2];
static uint32_t i2c_epm_last_write_ts;

static bool i2c_epm_read(uint16_t addr, uint8_t* buff, uint8_t sz)
{
	uint8_t address[2] = {addr >> 8, addr};
	HAL_StatusTypeDef rc = HAL_I2C_Master_Transmit(&hEPM_I2C, EPM_ADDRESS, address, 2, EPM_ADDR_TOUT);
	if (rc != HAL_OK)
		return false;
	rc = HAL_I2C_Master_Receive(&hEPM_I2C, EPM_ADDRESS, buff, sz, EPM_DATA_TOUT);
	return rc == HAL_OK;
}

static bool i2c_epm_write(uint16_t addr, uint8_t* buff, uint8_t sz)
{
	uint32_t const now = HAL_GetTick();
	uint32_t const elapsed_since_last = now - i2c_epm_last_write_ts;
	if (elapsed_since_last < EPM_WRITE_WAIT)
		HAL_Delay(EPM_WRITE_WAIT - elapsed_since_last);
	memmove(buff + 2, buff, sz);
	buff[0] = addr >> 8;
	buff[1] = addr;
	HAL_StatusTypeDef rc = HAL_I2C_Master_Transmit(&hEPM_I2C, EPM_ADDRESS, buff, 2 + sz, EPM_DATA_TOUT);
	i2c_epm_last_write_ts = HAL_GetTick();
	return rc == HAL_OK;
}

static int i2c_eeprom_wr_handler(const char* str, unsigned sz, struct scpi_node const* n)
{
	uint32_t addr;
	unsigned sz_ = sz;
	unsigned rc = scan_u(str, sz_, &addr);
	if (!rc || addr >= EPM_ADDR_END)
		return -err_param;
	str += rc;
	sz_ -= rc;
	uint8_t cnt;
	for (cnt = 0;; ++cnt) {
		uint32_t val;
		rc = scan_u(str, sz_, &val);
		if (!rc)
			break;
		if (val >= 0x100)
			return -err_param;
		if (cnt && !((addr + cnt) & (EPM_PAGE_SZ-1)))
			return -err_param;
		BUG_ON(cnt >= EPM_PAGE_SZ);
		i2c_epm_buff[cnt] = val;
		str += rc;
		sz_ -= rc;
	}
	if (!cnt)
		return -err_param;
	if (!i2c_epm_write(addr, i2c_epm_buff, cnt))
		return -err_internal;
	return sz - sz_;
}

static int i2c_eeprom_rd_handler(const char* str, unsigned sz, struct scpi_node const* n)
{
	uint32_t addr;
	unsigned rc = scan_u(str, sz, &addr);
	if (!rc || (addr >= EPM_ADDR_END) || (addr & (EPM_PAGE_SZ-1)))
		return -err_param;
	if (rc + 1 != sz || str[rc] != '?')
		return -err_cmd;
	if (!i2c_epm_read(addr, i2c_epm_buff, EPM_PAGE_SZ))
		return -err_internal;
	for (uint8_t i = 0; i < EPM_PAGE_SZ; ++i) {
		err_t err = cli_printf("%02X ", i2c_epm_buff[i]);
		if (err)
			return -err;
	}
	return sz;	
}

const struct scpi_node i2c_eeprom_nodes[] = {
	{
		"WR",
		NULL,
		i2c_eeprom_wr_handler,
		" ADDR BYTE0 [BYTE1 ..] writes up to 64 bytes starting at the ADDR",
	},
	{
		"RD",
		NULL,
		i2c_eeprom_rd_handler,
		" ADDR? returns 64-byte page starting at the ADDR",
	},
	SCPI_NODE_END
};
