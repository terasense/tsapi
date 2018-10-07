#ifndef TIMER_H
#define TIMER_H

// Initializes alarm timer generating ticks every 10 msec.
// After alarm_ticks the timer_alarm will be called unless the alarm time
// be updated by timer_alarm_update.
void timer_init(WORD alarm_ticks);

// Make sure the alarm won't fire sooner than the given number of ticks
void timer_alarm_update(WORD alarm_ticks);

// Alarm callback fired when alarm tick counter reach zero. Returns new counter value.
WORD timer_alarm(void);

#endif
