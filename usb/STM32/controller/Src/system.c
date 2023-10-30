#include "stm32f4xx_hal.h"
#include "system.h"
#include "main.h"
#include "io_util.h"
#include "version.h"
#include "test.h"

#include <string.h>
#include <intrinsics.h>

#define LOADER_MAGIC_KEY 0xdeadbeef

void Reboot_Loader(void);

__no_init unsigned sys_reset_magic;

const __root struct version_tag sys_fw_version_tag @ FW_TAG_ADDR = VERSION_TAG_INI;

/* Early stage initialization routine */
void sys_first_init(void)
{
	unsigned magic = sys_reset_magic;
	sys_reset_magic = 0;
	if (magic == LOADER_MAGIC_KEY)
		Reboot_Loader();
}

void sys_init(void)
{
	test_init();
	WRITE_PIN(ADS_RST, 0);
	WRITE_PIN(FX_nRST, 1);
}

void sys_run(void)
{
	test_run();
}

void _sys_schedule_bootloader(void)
{
	sys_reset_magic = LOADER_MAGIC_KEY;
}

void _sys_reset(void)
{
	IWDG_HandleTypeDef iwdg;
	iwdg.Instance = IWDG;
	iwdg.Init.Prescaler = IWDG_PRESCALER_4;
	iwdg.Init.Reload = 0;
	HAL_IWDG_Init(&iwdg);
	for (;;) __no_operation();
}

