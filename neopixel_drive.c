#include "neopixel_drive.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <util/atomic.h>

static void neo_drive_start_reset(neo_drive_t * d)
{
    /* Take control of pin and set to 0 */
    *d->port &= ~d->pinmask;

    swtimer_set(&d->timer, d->reset_period);
}

static void neo_drive_write_show(neo_drive_t * d)
{
    // WS2811 and WS2812 have different hi/lo duty cycles; this is
    // similar but NOT an exact copy of the prior 400-on-8 code.

    // 20 inst. clocks per bit: HHHHHxxxxxxxxLLLLLLL
    // ST instructions:         ^   ^        ^       (T=0,5,13)
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {

    volatile uint16_t i   = d->buf_size; // Loop counter
      volatile uint8_t
       *ptr = d->buf,   // Pointer to next byte
        b   = *ptr++,   // Current byte value
        hi,             // PORT w/output bit set high
        lo;             // PORT w/output bit set low
    volatile uint8_t next, bit;

    hi   = *d->port |  d->pinmask;
    lo   = *d->port & ~d->pinmask;
    next = lo;
    bit  = 8;

    asm volatile(
     "head20:"                   "\n\t" // Clk  Pseudocode    (T =  0)
      "cli"    					 "\n\t" //
      "st   %a[port],  %[hi]"    "\n\t" // 2    PORT = hi     (T =  2)
      "sbrc %[byte],  7"         "\n\t" // 1-2  if(b & 128)
       "mov  %[next], %[hi]"     "\n\t" // 0-1   next = hi    (T =  4)
      "dec  %[bit]"              "\n\t" // 1    bit--         (T =  5)
      "st   %a[port],  %[next]"  "\n\t" // 2    PORT = next   (T =  7)
      "mov  %[next] ,  %[lo]"    "\n\t" // 1    next = lo     (T =  8)
      "breq nextbyte20"          "\n\t" // 1-2  if(bit == 0) (from dec above)
      "rol  %[byte]"             "\n\t" // 1    b <<= 1       (T = 10)
      "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 12)
      "nop"                      "\n\t" // 1    nop           (T = 13)
      "st   %a[port],  %[lo]"    "\n\t" // 2    PORT = lo     (T = 15)
      "nop"                      "\n\t" // 1    nop           (T = 16)
      "rjmp .+0"                 "\n\t" // 2    nop nop       (T = 18)
      "rjmp head20"              "\n\t" // 2    -> head20 (next bit out)
     "nextbyte20:"               "\n\t" //                    (T = 10)
      "ldi  %[bit]  ,  8"        "\n\t" // 1    bit = 8       (T = 11)
      "ld   %[byte] ,  %a[ptr]+" "\n\t" // 2    b = *ptr++    (T = 13)
      "st   %a[port], %[lo]"     "\n\t" // 2    PORT = lo     (T = 15)
      "sei"                      "\n\t" // 1    sei           (T = 16)
      "sbiw %[count], 1"         "\n\t" // 2    i--           (T = 18)
       "brne head20"             "\n"   // 2    if(i != 0) -> (next byte)
      : [port]  "+e" (d->port),
        [byte]  "+r" (b),
        [bit]   "+r" (bit),
        [next]  "+r" (next),
        [count] "+w" (i)
      : [ptr]    "e" (ptr),
        [hi]     "r" (hi),
        [lo]     "r" (lo));
    }

    neo_drive_start_reset(d);
}

void neo_drive_init(neo_drive_t * d, uint16_t reset_period, volatile uint8_t * port, volatile uint8_t * ddr, uint8_t pinmask)
{
    memset(d, 0, sizeof(*d));

    d->port = port;
    d->pinmask = pinmask;

    *d->port &= d->pinmask;
    *ddr |= d->pinmask;

    neo_drive_start_reset(d);
}

/*! Starts a drive if not already started */
int neo_drive_start_show(neo_drive_t * d, size_t buf_size, void * buf)
{
    if(d->show_pending)
    {
        return -1;
    }

    d->buf_size = buf_size;
    d->buf = buf;
    d->show_pending = true;

    return 0;
}

/*! Get status of drive FSM */
neo_drive_status_t neo_drive_service(neo_drive_t * d)
{
    if(d->show_pending)
    {
        if(swtimer_is_expired(&d->timer))
        {
            neo_drive_write_show(d);
            d->show_pending = false;
            return NEO_COMPLETE;
        }
        else
        {
            return NEO_WAITING;
        }
    }
    else
    {
        return NEO_IDLE;
    }
}
