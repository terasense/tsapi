//-----------------------------------------------------------------------------
//   File:      fw.c
//   Contents:  Firmware frameworks task dispatcher and device request parser
//
// $Archive: /USB/Examples/FX2LP/bulkext/fw.c $
// $Date: 3/23/05 2:53p $
// $Revision: 8 $
//
//
//-----------------------------------------------------------------------------
// Copyright (c) 2011, Cypress Semiconductor Corporation All rights reserved
//-----------------------------------------------------------------------------

#include "fx2.h"
#include "fx2regs.h"
#include "syncdly.h"            // SYNCDELAY macro
#include "timer.h"

//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------
volatile BOOL   GotSUD;
volatile BOOL   Sleep;                  // Sleep mode enable flag
volatile BOOL   Reenum;                 // Re-enumeration request
BOOL            Rwuen;
BOOL            Selfpwr;

WORD   pDeviceDscr;   // Pointer to Device Descriptor; Descriptors may be moved
WORD   pDeviceQualDscr;
WORD   pHighSpeedConfigDscr;
WORD   pFullSpeedConfigDscr;
WORD   pConfigDscr;
WORD   pOtherConfigDscr;
WORD   pStringDscr;

//-----------------------------------------------------------------------------
// Prototypes
//-----------------------------------------------------------------------------
static void SetupCommand(void);

void TD_Init(void);
void TD_Poll(void);
BOOL TD_Suspend(void);
BOOL TD_Resume(void);

BOOL DR_GetDescriptor(void);
BOOL DR_SetConfiguration(void);
BOOL DR_GetConfiguration(void);
BOOL DR_SetInterface(void);
BOOL DR_GetInterface(void);
BOOL DR_GetStatus(void);
BOOL DR_ClearFeature(void);
BOOL DR_SetFeature(void);
BOOL DR_VendorCmnd(void);

// this table is used by the epcs macro 
const char code  EPCS_Offset_Lookup_Table[] =
{
   0,    // EP1OUT
   1,    // EP1IN
   2,    // EP2OUT
   2,    // EP2IN
   3,    // EP4OUT
   3,    // EP4IN
   4,    // EP6OUT
   4,    // EP6IN
   5,    // EP8OUT
   5,    // EP8IN
};

// macro for generating the address of an endpoint's control and status register (EPnCS)
#define epcs(EP) (EPCS_Offset_Lookup_Table[(EP & 0x7E) | (EP > 128)] + 0xE6A1)

//-----------------------------------------------------------------------------
// Code
//-----------------------------------------------------------------------------

// Task dispatcher
void main(void)
{
   // Initialize Global States
   Sleep = FALSE;               // Disable sleep mode
   Rwuen = FALSE;               // Disable remote wakeup
   Selfpwr = FALSE;             // Disable self powered
   Reenum  = FALSE;             // Re-enumeration request
   GotSUD = FALSE;              // Clear "Got setup data" flag

   // Initialize user device
   TD_Init();

   // Initialize descriptor pointers
   pDeviceDscr = (WORD)&DeviceDscr;
   pDeviceQualDscr = (WORD)&DeviceQualDscr;
   pHighSpeedConfigDscr = (WORD)&HighSpeedConfigDscr;
   pFullSpeedConfigDscr = (WORD)&FullSpeedConfigDscr;
   pStringDscr = (WORD)&StringDscr;

   EZUSB_IRQ_ENABLE();            // Enable USB interrupt (INT2)
   EZUSB_ENABLE_RSMIRQ();            // Wake-up interrupt

   INTSETUP |= (bmAV2EN | bmAV4EN);     // Enable INT 2 & 4 autovectoring

   USBIE |= bmSOF | bmSUDAV | bmSUTOK | bmSUSP | bmURES | bmHSGRANT;   // Enable selected interrupts
   EA = 1;                  // Enable 8051 interrupts

#ifndef NO_RENUM
   EZUSB_Discon(TRUE);   // renumerate
#endif

   // unconditionally re-connect.  If we loaded from eeprom we are
   // disconnected and need to connect.  If we just renumerated this
   // is not necessary but doesn't hurt anything
   USBCS &=~bmDISCON;

   CKCON = (CKCON&(~bmSTRETCH)) | FW_STRETCH_VALUE; // Set stretch

   // clear the Sleep flag.
   Sleep = FALSE;

   // Task Dispatcher
   while(TRUE)               // Main Loop
   {
      // Reenumerate if necessary
      if (Reenum)
      {
         EZUSB_Discon(TRUE);
         // Give the host 10 sec to enumerate
         timer_alarm_update(1000);
         Reenum = FALSE;
      }

      // Poll User Device
      TD_Poll();

      // Check for pending SETUP
      if(GotSUD)
      {
         SetupCommand();          // Implement setup command
         GotSUD = FALSE;          // Clear SETUP flag
      }

      // check for and handle suspend.
      // NOTE: Idle mode stops the processor clock.  There are only two
      // ways out of idle mode, the WAKEUP pin, and detection of the USB
      // resume state on the USB bus.  The timers will stop and the
      // processor will not wake up on any other interrupts.
      if (Sleep)
      {
         if(TD_Suspend())
         { 
            Sleep = FALSE;     // Clear the "go to sleep" flag.  Do it here to prevent any race condition between wakeup and the next sleep.
            do
            {
               EZUSB_Susp();         // Place processor in idle mode.
            }
            while(!Rwuen && EZUSB_EXTWAKEUP());
            // above.  Must continue to go back into suspend if the host has disabled remote wakeup
            // *and* the wakeup was caused by the external wakeup pin.

            // 8051 activity will resume here due to USB bus or Wakeup# pin activity.
            EZUSB_Resume();   // If source is the Wakeup# pin, signal the host to Resume.      
            TD_Resume();
         }
      }
   }
}

BOOL HighSpeedCapable()
{
   // this function determines if the chip is high-speed capable.
   // FX2 and FX2LP are high-speed capable. FX1 is not - it does
   // not have a high-speed transceiver.

   if (GPCR2 & bmFULLSPEEDONLY)
      return FALSE;
   else
      return TRUE;
}

void StallEP0(void)
{
   Reenum = TRUE;
}

#define SET_LINE_CODING (0x20)
#define GET_LINE_CODING (0x21)
#define SET_CONTROL_STATE (0x22)

/*
Line coding structure:
Offset Field       Size Value   Description
0      dwDTERate   4    Number  Data terminal rate, in bits per second.
4      bCharFormat 1    Number  Stop bits: 0 - 1 Stop bit, 1 - 1.5 Stop bits, 2 - 2 Stop bits
5      bParityType 1    Number  Parity: 0 - None, 1 - Odd, 2 - Even, 3 - Mark, 4 - Space
6      bDataBits   1    Number  Data bits (5, 6, 7, 8 or 16)
*/

// 2400 baud, no parity, 1 stop bit
BYTE xdata LineCode[7] = {0x60,0x09,0x00,0x00,0x00,0x00,0x08};

static void CDC_SetLineEncoding(void)
{
   EP0BCL = 0x00;
}

static void CDC_GetLineCoding(void)
{
   SUDPTRCTL = 0x01;
   SUDPTRH = MSB(LineCode);
   SUDPTRL = LSB(LineCode);
}

// Device request parser
void SetupCommand(void)
{
   void   *dscr_ptr;

   switch(SETUPDAT[1])
   {
      case SET_LINE_CODING:
         CDC_SetLineEncoding();
         break;

      case GET_LINE_CODING:
         CDC_GetLineCoding();
         break;

      case SET_CONTROL_STATE:
/*
Control Signal Bitmap Values:
Bit position Description
15..2        RESERVED
1            RTS
0            DTR
*/
         break;

      case SC_GET_DESCRIPTOR:                  // *** Get Descriptor
         SUDPTRCTL = 0x01;
         if(DR_GetDescriptor())
            switch(SETUPDAT[3])         
            {
               case GD_DEVICE:            // Device
                  SUDPTRH = MSB(pDeviceDscr);
                  SUDPTRL = LSB(pDeviceDscr);
                  break;
               case GD_DEVICE_QUALIFIER:            // Device Qualifier
                  // only return a device qualifier if this is a high speed
                  // capable chip.
                  if (HighSpeedCapable())
                  {
                     SUDPTRH = MSB(pDeviceQualDscr);
                     SUDPTRL = LSB(pDeviceQualDscr);
                  }
                  else
                  {
                     StallEP0();
                  }
                  break;
               case GD_CONFIGURATION:         // Configuration
                  SUDPTRH = MSB(pConfigDscr);
                  SUDPTRL = LSB(pConfigDscr);
                  break;
               case GD_OTHER_SPEED_CONFIGURATION:  // Other Speed Configuration
                  SUDPTRH = MSB(pOtherConfigDscr);
                  SUDPTRL = LSB(pOtherConfigDscr);
                  break;
               case GD_STRING:            // String
                  if(dscr_ptr = (void *)EZUSB_GetStringDscr(SETUPDAT[2]))
                  {
                     SUDPTRH = MSB(dscr_ptr);
                     SUDPTRL = LSB(dscr_ptr);
                  }
                  else 
                     StallEP0();   // Stall End Point 0
                  break;
               default:            // Invalid request
                  StallEP0();      // Stall End Point 0
            }
         break;
      case SC_GET_INTERFACE:                  // *** Get Interface
         DR_GetInterface();
         break;
      case SC_SET_INTERFACE:                  // *** Set Interface
         DR_SetInterface();
         break;
      case SC_SET_CONFIGURATION:               // *** Set Configuration
         DR_SetConfiguration();
         break;
      case SC_GET_CONFIGURATION:               // *** Get Configuration
         DR_GetConfiguration();
         break;
      case SC_GET_STATUS:                  // *** Get Status
         if(DR_GetStatus())
            switch(SETUPDAT[0])
            {
               case GS_DEVICE:            // Device
                  EP0BUF[0] = ((BYTE)Rwuen << 1) | (BYTE)Selfpwr;
                  EP0BUF[1] = 0;
                  EP0BCH = 0;
                  EP0BCL = 2;
                  break;
               case GS_INTERFACE:         // Interface
                  EP0BUF[0] = 0;
                  EP0BUF[1] = 0;
                  EP0BCH = 0;
                  EP0BCL = 2;
                  break;
               case GS_ENDPOINT:         // End Point
                  EP0BUF[0] = *(BYTE xdata *) epcs(SETUPDAT[4]) & bmEPSTALL;
                  EP0BUF[1] = 0;
                  EP0BCH = 0;
                  EP0BCL = 2;
                  break;
               default:            // Invalid Command
                  StallEP0();      // Stall End Point 0
            }
         break;
      case SC_CLEAR_FEATURE:                  // *** Clear Feature
         if(DR_ClearFeature())
            switch(SETUPDAT[0])
            {
               case FT_DEVICE:            // Device
                  if(SETUPDAT[2] == 1)
                     Rwuen = FALSE;       // Disable Remote Wakeup
                  else
                     StallEP0();   // Stall End Point 0
                  break;
               case FT_ENDPOINT:         // End Point
                  if(SETUPDAT[2] == 0)
                  {
                     *(BYTE xdata *) epcs(SETUPDAT[4]) &= ~bmEPSTALL;
                     EZUSB_RESET_DATA_TOGGLE( SETUPDAT[4] );
                  }
                  else
                     StallEP0();   // Stall End Point 0
                  break;
            }
         break;
      case SC_SET_FEATURE:                  // *** Set Feature
         if(DR_SetFeature())
            switch(SETUPDAT[0])
            {
               case FT_DEVICE:            // Device
                  if(SETUPDAT[2] == 1)
                     Rwuen = TRUE;      // Enable Remote Wakeup
                  else if(SETUPDAT[2] == 2)
                     // Set Feature Test Mode.  The core handles this request.  However, it is
                     // necessary for the firmware to complete the handshake phase of the
                     // control transfer before the chip will enter test mode.  It is also
                     // necessary for FX2 to be physically disconnected (D+ and D-)
                     // from the host before it will enter test mode.
                     break;
                  else
                     StallEP0();   // Stall End Point 0
                  break;
               case FT_ENDPOINT:         // End Point
                  *(BYTE xdata *) epcs(SETUPDAT[4]) |= bmEPSTALL;
                  break;
               default:
                  StallEP0();      // Stall End Point 0
            }
         break;
      default:                     // *** Invalid Command
         if(DR_VendorCmnd())
            StallEP0();            // Stall End Point 0
   }

   // Acknowledge handshake phase of device request
   EP0CS |= bmHSNAK;
}

// Wake-up interrupt handler
void resume_isr(void) interrupt WKUP_VECT
{
   EZUSB_CLEAR_RSMIRQ();
}


