#include "main.h"
#include "cli.h"
#include "usbd_cdc_if.h"

void cli_receive(uint8_t* Buf, uint32_t *Len)
{
	CDC_Transmit_FS(Buf, *Len);
}

