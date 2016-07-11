#include "swtimer.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

static volatile uint32_t now;

#define _TIMER_ISR_VEC(t) TIMER ## t ## _COMPA_vect
#define TIMER_ISR_VEC(t) _TIMER_ISR_VEC(t)

#define _TCCRA(t) TCCR ## t ## A
#define TCCRA(t) _TCCRA(t)

#define _TCCRB(t) TCCR ## t ## B
#define TCCRB(t) _TCCRB(t)

#define _TCCRC(t) TCCR ## t ## C
#define TCCRC(t) _TCCRC(t)

#define _OCRA(t) OCR ## t ## A
#define OCRA(t) _OCRA(t)

#define _TIMSK(t) TIMSK ## t
#define TIMSK(t) _TIMSK(t)

#define _TIFR(t) TIFR ## t
#define TIFR(t) _TIFR(t)

#define _TCNT(t) TCNT ## t
#define TCNT(t) _TCNT(t)

ISR(TIMER_ISR_VEC(SWTIMER_TIMER))
{
	now += 1;
}

void swtimer_setup(void)
{
	TCCRA(SWTIMER_TIMER) = 0;
	TCCRB(SWTIMER_TIMER) = /*_BV(WGM12)|  */_BV(CS01);
	TCCRC(SWTIMER_TIMER) = 0;
	//OCRA(SWTIMER_TIMER) = F_CPU/1000/8;
	TIMSK(SWTIMER_TIMER) = _BV(OCIE1A);
}

static void swtimer_get_time(uint32_t * lnow, uint16_t * usec2)
{
    uint8_t tifr;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        *lnow = now;
        *usec2 = TCNT(SWTIMER_TIMER);
        tifr = TIFR(SWTIMER_TIMER);
    }

    /* There was a pending overflow, handle it */
    if(tifr & _BV(OCIE1A))
    {
        lnow += 1;

        /* Timer wrapped between TCNT and TIFR read */
        if(*usec2 > 0xFFF0)
        {
            *usec2 = 0;
        }
    }
}

uint32_t swtimer_now_usec()
{
    uint16_t usec2;
    uint32_t lnow;

    swtimer_get_time(&lnow, &usec2);

    return (lnow << 15) | (usec2 >> 1);
}

uint32_t swtimer_now_msec()
{
    uint16_t usec2;
    uint32_t lnow;

    swtimer_get_time(&lnow, &usec2);

    return ((lnow << 12) | (usec2 >> 4))/125;
}

void swtimer_set(swtimer_t * s, uint32_t expiration)
{
	s->state = SWTIMER_SET;
	s->expiration = swtimer_now_usec() + expiration;
}

int swtimer_is_expired(swtimer_t * s)
{
	if(s->state == SWTIMER_SET && swtimer_now_usec() > s->expiration)
	{
		s->state = SWTIMER_EXPIRED;
	}

	return s->state == SWTIMER_EXPIRED;
}
