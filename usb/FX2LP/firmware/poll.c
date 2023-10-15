//-----------------------------------------------------------------------------
//   File:      bulkloop.c
//   Contents:  Hooks required to implement USB peripheral function.
//
// $Archive: /USB/Examples/FX2LP/bulkloop/bulkloop.c $
//
//
//-----------------------------------------------------------------------------
// Copyright (c) 2011, Cypress Semiconductor Corporation All rights reserved
//-----------------------------------------------------------------------------
#pragma NOIV               // Do not generate interrupt vectors

#include "fx2.h"
#include "fx2regs.h"
#include "syncdly.h"            // SYNCDELAY macro
#include "timer.h"
#include "main.h"
#include "poll.h"

static BYTE Configuration;             // Current configuration
static BYTE AlternateSetting;          // Alternate settings
static BYTE HighSpeed;

// Alarm callback fired when alarm tick counter reach zero. Returns new counter value.
WORD timer_alarm(void)
{
	Reenum = TRUE;
	return 1000; // 10 sec
}

//-----------------------------------------------------------------------------
// Task Dispatcher hooks
//   The following hooks are called by the task dispatcher.
//-----------------------------------------------------------------------------

void TD_Init(void)             // Called once at startup
{
	// set the CPU clock to 48MHz
	CPUCS = ((CPUCS & ~bmCLKSPD) | bmCLKSPD1) ;

	OEA = PA0_LED | PA1_LED | PA2_nSLOE | PA4_FIFOADDR0 | PA5_FIFOADDR1;
	IOA = PA0_LED | PA1_LED | PA2_nSLOE | PA5_FIFOADDR1;

	// defines the external interface as follows:
	// use internal IFCLK (48MHz)
	// use slave FIFO interface pins asynchronously to external master
#define CLK_INT   0x80
#define CLK_48MHz 0x40
#define CLK_OE    0x20
#define CLK_INV   0x10
#define FIFO_ASYNC 8
#define FIFO_GPIF  2
#define FIFO_SLAVE 3
	IFCONFIG = CLK_INT + CLK_48MHz + FIFO_ASYNC + FIFO_SLAVE;

	// Registers which require a synchronization delay, see section 15.14
	// FIFORESET        FIFOPINPOLAR
	// INPKTEND         OUTPKTEND
	// EPxBCH:L         REVCTL
	// GPIFTCB3         GPIFTCB2
	// GPIFTCB1         GPIFTCB0
	// EPxFIFOPFH:L     EPxAUTOINLENH:L
	// EPxFIFOCFG       EPxGPIFFLGSEL
	// PINFLAGSxx       EPxFIFOIRQ
	// EPxFIFOIE        GPIFIRQ
	// GPIFIE           GPIFADRH:L
	// UDMACRCH:L       EPxGPIFTRIG
	// GPIFTRIG

	// Note: The pre-REVE EPxGPIFTCH/L register are affected, as well...
	//      ...these have been replaced by GPIFTC[B3:B0] registers

	// default: all endpoints have their VALID bit set
	// default: TYPE1 = 1 and TYPE0 = 0 --> BULK  
	// default: EP2 and EP4 DIR bits are 0 (OUT direction)
	// default: EP6 and EP8 DIR bits are 1 (IN direction)
	// default: EP2, EP4, EP6, and EP8 are double buffered

	// we are just using the default values, yes this is not necessary...
	SYNCDELAY; EP1OUTCFG = 0xA0;
	SYNCDELAY; EP1INCFG = 0xB0;     // Configure EP1IN as BULK IN EP
	SYNCDELAY; EP2CFG = 0xA2;       // Double buffer OUT EP
	SYNCDELAY; EP4CFG &= 0x7F;      // Invalid EP
	SYNCDELAY; EP6CFG = 0xE0;       // Quad buffer IN EP
	SYNCDELAY; EP8CFG &= 0x7F;      // Invalid EP

	// out endpoints do not come up armed
	// since the defaults are double buffered we must write dummy byte counts (setting SKIP bit) twice
	SYNCDELAY; EP2BCL = 0x80;                // arm EP2OUT by writing byte count w/skip.
	SYNCDELAY; EP2BCL = 0x80;

#ifndef TEST_LOOPBACK
	SYNCDELAY; EP6FIFOCFG = 4 + 1; // ZEROLENIN WORDWIDE

	// Reset the FIFO
	SYNCDELAY; FIFORESET = 0x80;
	SYNCDELAY; FIFORESET = 0x82;
	SYNCDELAY; FIFORESET = 0x86;
	SYNCDELAY; FIFORESET = 0x00;

	SYNCDELAY; EP6FIFOCFG |= 8; // AUTOIN
	SYNCDELAY; EP6AUTOINLENH = 0x02; // Auto-commit 512-byte packets
	SYNCDELAY; EP6AUTOINLENL = 0x00;
#define PKST0 0x08
#define PKST1 0x10
#define PFC8  1
	// PF is high when FIFO has at most 3 committed packets and 256 (PFC8)
	// uncommitted so we can safely write yet another 256 bytes
	SYNCDELAY; EP6FIFOPFH = PKST0 + PKST1 + PFC8;
	SYNCDELAY; EP6FIFOPFL = 0;
#endif

	Rwuen = TRUE;                 // Enable remote-wakeup

	// enable dual autopointer feature
	AUTOPTRSETUP |= 0x01;

	// Enable 100Hz timer
	timer_init(1000); // 10 sec to alarm

	HighSpeed = FALSE;
}

void TD_Poll(void)   // Called repeatedly while the device is idle
{
	if(!(EP2468STAT & bmEP2EMPTY))		// Is EP2-OUT buffer not empty (has at least one packet)?
	{
#ifdef TEST_LOOPBACK
		if(!(EP2468STAT & bmEP6FULL))	// YES: Is EP6-IN buffer not full (room for at least 1 pkt)?
		{
			WORD i;
			WORD const count = (EP2BCH << 8) + EP2BCL;

			APTR1H = MSB( &EP2FIFOBUF );
			APTR1L = LSB( &EP2FIFOBUF );
			AUTOPTRH2 = MSB( &EP6FIFOBUF );
			AUTOPTRL2 = LSB( &EP6FIFOBUF );

			// loop EP2OUT buffer data to EP6IN
			for( i = 0; i < count; i++ )
			{
				EXTAUTODAT2 = EXTAUTODAT1;	// Autopointers make block transfers easy...
			}
			EP6BCH = EP2BCH;		// Send the same number of bytes as received  
			SYNCDELAY;  
			EP6BCL = EP2BCL;        // arm EP6IN
			SYNCDELAY;                    
#ifdef DEBUG_LEDS
			IOA ^= PA1_LED;
#endif
#else
		{
#endif
			EP2BCL = 0x80;          // arm EP2OUT
		}
	}
}

BOOL TD_Suspend(void)          // Called before the device goes into suspend mode
{
	return(TRUE);
}

BOOL TD_Resume(void)          // Called after the device resumes
{
	return(TRUE);
}

//-----------------------------------------------------------------------------
// Device Request hooks
//   The following hooks are called by the end point 0 device request parser.
//-----------------------------------------------------------------------------

BOOL DR_GetDescriptor(void)
{
	return(TRUE);
}

BOOL DR_SetConfiguration(void)   // Called when a Set Configuration command is received
{
	Configuration = SETUPDAT[2];
	return(TRUE);            // Handled by user code
}

BOOL DR_GetConfiguration(void)   // Called when a Get Configuration command is received
{
	EP0BUF[0] = Configuration;
	EP0BCH = 0;
	EP0BCL = 1;
	return(TRUE);            // Handled by user code
}

BOOL DR_SetInterface(void)       // Called when a Set Interface command is received
{
	AlternateSetting = SETUPDAT[2];
	return(TRUE);            // Handled by user code
}

BOOL DR_GetInterface(void)       // Called when a Set Interface command is received
{
	EP0BUF[0] = AlternateSetting;
	EP0BCH = 0;
	EP0BCL = 1;
	return(TRUE);            // Handled by user code
}

BOOL DR_GetStatus(void)
{
	return(TRUE);
}

BOOL DR_ClearFeature(void)
{
	return(TRUE);
}

BOOL DR_SetFeature(void)
{
	return(TRUE);
}

BOOL DR_VendorCmnd(void)
{
  BYTE tmp;
  
  switch (SETUPDAT[1])
  {
	  case VR_NAKALL_ON:
		  tmp = FIFORESET;
		  tmp |= bmNAKALL;      
		  SYNCDELAY;                    
		  FIFORESET = tmp;
		  break;
	  case VR_NAKALL_OFF:
		  tmp = FIFORESET;
		  tmp &= ~bmNAKALL;      
		  SYNCDELAY;                    
		  FIFORESET = tmp;
		  break;
	  default:
		  return(TRUE);
  }

  return(FALSE);
}

//-----------------------------------------------------------------------------
// USB Interrupt Handlers
//   The following functions are called by the USB interrupt jump table.
//-----------------------------------------------------------------------------

// Setup Data Available Interrupt Handler
void ISR_Sudav(void) interrupt 0
{
	GotSUD = TRUE;            // Set flag
	EZUSB_IRQ_CLEAR();
	USBIRQ = bmSUDAV;         // Clear SUDAV IRQ
}

// Setup Token Interrupt Handler
void ISR_Sutok(void) interrupt 0
{
	EZUSB_IRQ_CLEAR();
	USBIRQ = bmSUTOK;         // Clear SUTOK IRQ
}

#ifdef DEBUG_LEDS
static BYTE sof_cnt;
#endif

void ISR_Sof(void) interrupt 0
{
	EZUSB_IRQ_CLEAR();
	USBIRQ = bmSOF;            // Clear SOF IRQ

#ifdef DEBUG_LEDS
	if (!sof_cnt++)
		IOA ^= PA0_LED;
#endif
	if (FNADDR != 0) {
		timer_alarm_update(10);
	}
}

void ISR_Ures(void) interrupt 0
{
	HighSpeed = FALSE;
#ifndef TEST_LOOPBACK
	IOA |= PA1_nHS;
#endif
	// whenever we get a USB reset, we should revert to full speed mode
	pConfigDscr = pFullSpeedConfigDscr;
	((CONFIGDSCR xdata *) pConfigDscr)->type = CONFIG_DSCR;
	pOtherConfigDscr = pHighSpeedConfigDscr;
	((CONFIGDSCR xdata *) pOtherConfigDscr)->type = OTHERSPEED_DSCR;

	EZUSB_IRQ_CLEAR();
	USBIRQ = bmURES;         // Clear URES IRQ
}

void ISR_Susp(void) interrupt 0
{
	Sleep = TRUE;
	EZUSB_IRQ_CLEAR();
	USBIRQ = bmSUSP;
}

void ISR_Highspeed(void) interrupt 0
{
	if (EZUSB_HIGHSPEED())
	{
		HighSpeed = TRUE;
#ifndef TEST_LOOPBACK
		IOA &= ~PA1_nHS;
#endif
		pConfigDscr = pHighSpeedConfigDscr;
		((CONFIGDSCR xdata *) pConfigDscr)->type = CONFIG_DSCR;
		pOtherConfigDscr = pFullSpeedConfigDscr;
		((CONFIGDSCR xdata *) pOtherConfigDscr)->type = OTHERSPEED_DSCR;
	}

	EZUSB_IRQ_CLEAR();
	USBIRQ = bmHSGRANT;
}
void ISR_Ep0ack(void) interrupt 0
{
}
void ISR_Stub(void) interrupt 0
{
}
void ISR_Ep0in(void) interrupt 0
{
}
void ISR_Ep0out(void) interrupt 0
{
}
void ISR_Ep1in(void) interrupt 0
{
}
void ISR_Ep1out(void) interrupt 0
{
}
void ISR_Ep2inout(void) interrupt 0
{
}
void ISR_Ep4inout(void) interrupt 0
{
}
void ISR_Ep6inout(void) interrupt 0
{
}
void ISR_Ep8inout(void) interrupt 0
{
}
void ISR_Ibn(void) interrupt 0
{
}
void ISR_Ep0pingnak(void) interrupt 0
{
}
void ISR_Ep1pingnak(void) interrupt 0
{
}
void ISR_Ep2pingnak(void) interrupt 0
{
}
void ISR_Ep4pingnak(void) interrupt 0
{
}
void ISR_Ep6pingnak(void) interrupt 0
{
}
void ISR_Ep8pingnak(void) interrupt 0
{
}
void ISR_Errorlimit(void) interrupt 0
{
}
void ISR_Ep2piderror(void) interrupt 0
{
}
void ISR_Ep4piderror(void) interrupt 0
{
}
void ISR_Ep6piderror(void) interrupt 0
{
}
void ISR_Ep8piderror(void) interrupt 0
{
}
void ISR_Ep2pflag(void) interrupt 0
{
}
void ISR_Ep4pflag(void) interrupt 0
{
}
void ISR_Ep6pflag(void) interrupt 0
{
}
void ISR_Ep8pflag(void) interrupt 0
{
}
void ISR_Ep2eflag(void) interrupt 0
{
}
void ISR_Ep4eflag(void) interrupt 0
{
}
void ISR_Ep6eflag(void) interrupt 0
{
}
void ISR_Ep8eflag(void) interrupt 0
{
}
void ISR_Ep2fflag(void) interrupt 0
{
}
void ISR_Ep4fflag(void) interrupt 0
{
}
void ISR_Ep6fflag(void) interrupt 0
{
}
void ISR_Ep8fflag(void) interrupt 0
{
}
void ISR_GpifComplete(void) interrupt 0
{
}
void ISR_GpifWaveform(void) interrupt 0
{
}
