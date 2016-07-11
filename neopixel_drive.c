#include "neopixel_drive.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <util/atomic.h>

static void neo_drive_start_reset(neo_drive_t * d)
{
    /* Take control of pin and set to 0 */
    *d->uart->ucsrb = 0;
    *d->port &= ~d->pinmask;

    swtimer_set(&d->timer, d->reset_period);
}

#define L00 0x88
#define L01 0x8C
#define L10 0xC8
#define L11 0xCC

static inline void neo_drive_write_8bits(uint8_t v, uart_t * uart)
{
    for(size_t i = 0; i < 4; ++i)
    {
        while( !(*uart->ucsra & _BV(UDRE0)) );

        switch((v >> (6-2*i)) & 0x3)
        {
        case 0x0: *uart->udr = L00; break;
        case 0x1: *uart->udr = L01; break;
        case 0x2: *uart->udr = L10; break;
        case 0x3: *uart->udr = L11; break;
        }
    }
}

static void neo_drive_write_show(neo_drive_t * d)
{
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        *d->uart->ucsrb = _BV(TXEN0);

        for(size_t i = 0; i < d->buf_size; ++i)
        {
            neo_drive_write_8bits(d->buf[i], d->uart);
        }

        neo_drive_start_reset(d);
    }
}

static void neo_drive_setup_mspim(uart_t * uart)
{
    uart_disable(uart);

    /* Wait for tx in progress to clear out */
    while( !(*uart->ucsra & _BV(UDRE0)) );

    *uart->ubrrl = 0;
    *uart->ubrrh = 0;

    /*
    switch(uart->num)
    {
    case 0: XCK0_DDR |= _BV(XCK0); break;
    case 1: XCK1_DDR |= _BV(XCK1); break;
    case 2: XCK2_DDR |= _BV(XCK2); break;
    case 3: XCK3_DDR |= _BV(XCK3); break;
    }
    */

    *uart->ucsrc = _BV(UMSEL01) | _BV(UMSEL00);
    *uart->ucsrb = _BV(TXEN0);

#define NEO_BAUD ((F_CPU/2/4000000)-1)

#if NEO_BAUD < 1
#error "CPU frequency too slow!"
#endif

    *uart->ubrrh = NEO_BAUD >> 8;
    *uart->ubrrl = NEO_BAUD & 0xFF;
}

void neo_drive_init(neo_drive_t * d, uart_t * uart, uint8_t reset_period, volatile uint8_t * port, volatile uint8_t * ddr, uint8_t pinmask)
{
    memset(d, 0, sizeof(*d));

    d->uart = uart;

    d->port = port;
    d->pinmask = pinmask;

    *d->port &= d->pinmask;
    *ddr |= d->pinmask;

    neo_drive_setup_mspim(uart);
    neo_drive_start_reset(d);
}

/*! Starts a drive if not already started */
int neo_drive_start(neo_drive_t * d, size_t buf_size, void * buf)
{
    if(d->show_pending)
    {
        return -1;
    }

    d->buf_size = buf_size;
    d->buf = buf;

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
