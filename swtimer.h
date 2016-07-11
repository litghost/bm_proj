#ifndef _SWTIMER_H_
#define _SWTIMER_H_

#include <stdint.h>

/* This library uses one of the hardware timers to start a ~1 usec tick, SWTIMER_TIMER determines which one */
#ifndef SWTIMER_TIMER
#define SWTIMER_TIMER 1
#endif

typedef enum
{
	SWTIMER_UNSET,
	SWTIMER_SET,
	SWTIMER_EXPIRED,
} swtimer_state_t;

typedef struct 
{
	swtimer_state_t state;
	uint32_t expiration;
} swtimer_t;

void swtimer_setup(void);
void swtimer_set(swtimer_t * s, uint32_t expiration);
int swtimer_is_expired(swtimer_t * s);

uint32_t swtimer_now_usec();
uint32_t swtimer_now_msec();

#endif /* _SWTIMER_H_ */
