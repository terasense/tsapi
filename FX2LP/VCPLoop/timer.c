#include "fx2.h"
#include "fx2regs.h"

// 10ms interrupt
#define TIMER0_COUNT 0x63C0  // 48,000,000 Hz / (12 * 100)
#define TIMER0_ALARM_INI 500 // 5 sec

volatile WORD timer_alarm_cnt;

void timer_alarm(void);

void timer_alarm_reset(WORD val)
{
   EA = 0; // disables all interrupts
   timer_alarm_cnt = val;
   EA = 1; // enables all interrupts
}

// Timer Interrupt
// This function is an interrupt service routine for Timer 0. It should never
// be called by a C or assembly function. It will be executed automatically
// when Timer 0 overflows.
// "interrupt 1" tells the compiler to look for this ISR at address 000Bh
// "using 1" tells the compiler to use register bank 1
void timer0 (void) interrupt 1 using 1
{
	// Stop Timer 0, adjust the Timer 0 counter so that we get another
	// in 10ms, and restart the timer.
	TR0 = 0; // stop timer
	TL0 = TL0 + (TIMER0_COUNT & 0x00FF);
	TH0 = TH0 + (TIMER0_COUNT >> 8);

   if (!--timer_alarm_cnt) {
      timer_alarm_cnt = TIMER0_ALARM_INI;
      timer_alarm();
   }

	TR0 = 1; // start Timer 0
}

// This function enables Timer 0. Timer 0 generates a synchronous interrupt
// once every 100Hz or 10 ms.
void timer_init(void)
{	
	EA = 0; // disables all interrupts

	TR0 = 0; // stops Timer 0
	CKCON = 0x03; // Timer 0 using CLKOUT/12
	TMOD &= ~0x0F; // clear Timer 0 mode bits
	TMOD |= 0x01; // setup Timer 0 as a 16-bit timer
	TL0 = (TIMER0_COUNT & 0x00FF); // loads the timer counts
	TH0 = (TIMER0_COUNT >> 8);
	PT0 = 0; // sets the Timer 0 interrupt to low priority
	ET0 = 1; // enables Timer 0 interrupt
	TR0 = 1; // starts Timer 0

   timer_alarm_cnt = TIMER0_ALARM_INI;

	EA = 1; // enables all interrupts
}
