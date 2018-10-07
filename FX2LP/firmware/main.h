#ifndef MAIN_H
#define MAIN_H

// Flags:
extern volatile BOOL GotSUD; // setup data received
extern volatile BOOL Sleep;
extern volatile BOOL Reenum;
extern          BOOL Rwuen;
extern          BOOL Selfpwr;

// Pointers to descriptors
extern WORD   pDeviceDscr; 
extern WORD   pDeviceQualDscr;
extern WORD   pHighSpeedConfigDscr;
extern WORD   pFullSpeedConfigDscr;
extern WORD   pConfigDscr;
extern WORD   pOtherConfigDscr;
extern WORD   pStringDscr;

#endif
