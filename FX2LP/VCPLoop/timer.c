#include "fx2.h"
#include "fx2regs.h"

// 10ms interrupt
#define TIMER0_COUNT 0x63C0  // 48,000,000 Hz / (12 * 100)

// Alarm counter decremented every 10 msec
volatile WORD timer_alarm_cnt;

// Alarm callback fired when alarm tick counter reach zero. Returns new counter value.
WORD timer_alarm(void);

// Make sure the alarm won't fire sooner than the given number of ticks
void timer_alarm_update(WORD alarm_ticks)
{
   EA = 0; // disables all interrupts
   if (timer_alarm_cnt < alarm_ticks)
      timer_alarm_cnt = alarm_ticks;
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
      timer_alarm_cnt = timer_alarm();
   }

	TR0 = 1; // start Timer 0
}

// Initializes alarm timer generating ticks every 10 msec.
// After alarm_ticks the timer_alarm will be called unless the alarm time
// be updated by timer_alarm_update.
void timer_init(WORD alarm_ticks)
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

   timer_alarm_cnt = alarm_ticks;

	EA = 1; // enables all interrupts
}
