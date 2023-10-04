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

#define MAX_PACKET (HighSpeed ? 512 : 64)

#define VR_NAKALL_ON    0xD0
#define VR_NAKALL_OFF   0xD1

#define SET_LINE_CODING   (0x20)
#define GET_LINE_CODING   (0x21)
#define SET_CONTROL_STATE (0x22)
#define SEND_LINE_BREAK   (0x23)

/*
Line coding structure:
Offset Field       Size Value   Description
0      dwDTERate   4    Number  Data terminal rate, in bits per second.
4      bCharFormat 1    Number  Stop bits: 0 - 1 Stop bit, 1 - 1.5 Stop bits, 2 - 2 Stop bits
5      bParityType 1    Number  Parity: 0 - None, 1 - Odd, 2 - Even, 3 - Mark, 4 - Space
6      bDataBits   1    Number  Data bits (5, 6, 7, 8 or 16)
*/

// 2400 baud, no parity, 1 stop bit
#define DEF_LINE_CODING {0x60,0x09,0x00,0x00,0x00,0x00,0x08}

//#define TEST_LOOPBACK
#define DEBUG_LEDS

#endif
