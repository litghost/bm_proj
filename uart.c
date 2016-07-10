#include "uart.h"

#include <avr/interrupt.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>

static uart_t * volatile uarts[4];

void _uart_init(uart_t * uart, 
        int uart_num,
        size_t rx_buf_size, void * rx_buf, 
        size_t tx_buf_size, void * tx_buf,
        volatile uint8_t * udr, 
        volatile uint8_t * ucsra, volatile uint8_t * ucsrb, volatile uint8_t * ucsrc,
        volatile uint8_t * ubrrl, volatile uint8_t * ubrrh)
{
    assert(uart_num < sizeof(uarts)/sizeof(uarts[0]));
    memset(uart, 0, sizeof(*uart));

    buf_init(&uart->rx_buf, rx_buf_size, rx_buf);
    buf_init(&uart->tx_buf, tx_buf_size, tx_buf);

    uart->udr = udr;
    uart->ucsra = ucsra;
    uart->ucsrb = ucsrb;
    uart->ucsrc = ucsrc;
    uart->ubrrl = ubrrl;
    uart->ubrrh = ubrrh;

    /* Stop UART until configured */
    *uart->ucsrb = 0;
    *uart->ucsra = _BV(U2X0); /* Use 8x divider instead of 16x for higher baud rates *//
    *uart->ucsrc = _BV(UCSZ01) | _BV(UCSZ00); /* UART 8N1 */

    /* Drain Rx FIFO */
    volatile uint8_t dummy = *uart->udr;
    dummy = *uart->udr;
    (void)dummy;

    uart->num = uart_num;
    uarts[uart_num] = NULL;
}

static inline void uart_set_rts(uart_t * uart, bool rts)
{
    if(!uart->port_rts) 
    {
        return;
    }

    /* Low means asserted, so invert */
    if(!rts)
    {
        *uart->port_rts |= _BV(uart->iport_rts);
    }
    else
    {
        *uart->port_rts &= ~_BV(uart->iport_rts);
    }
}

static inline int uart_cts(uart_t * uart)
{
    if(!uart->pin_cts)
    {
        /* No HW flow, always clear to send */
        return 1;
    }
    else
    {
        uint8_t v = *uart->pin_cts & _BV(uart->ipin_cts);

        /* Low means asserted, so invert */
        return !(v != 0);
    }
}

void _uart_set_hardware_flow(uart_t * uart, 
        uint8_t ipin_cts, volatile uint8_t * pin_cts, volatile uint8_t * ddr_cts, volatile uint8_t * port_cts,
        uint8_t iport_rts, volatile uint8_t * port_rts, volatile uint8_t * ddr_rts)
{
    /* Disable UART before messing with anything */
    uart_disable(uart);

    /* Make CTS input, hi-Z */
    *ddr_cts &= ~_BV(ipin_cts);
    *port_cts &= ~_BV(ipin_cts);

    /* Make RTS output */
    *ddr_rts |= _BV(iport_rts);

    uart->ipin_cts = ipin_cts;
    uart->pin_cts = pin_cts;
    uart->iport_rts = iport_rts;
    uart->port_rts = port_rts;

    /* Initial stop serial traffic until enabled */
    uart_set_rts(uart, false);
}

int uart_set_baudrate(uart_t * uart, long baud)
{
    uart_disable(uart);

    uint32_t reg = F_CPU/8/baud - 1;

    *uart->ubrrl = (reg >> 0) & 0xFF;
    *uart->ubrrh = (reg >> 8) & 0xFF;

    return 0;
}

static void uart_update_rts(uart_t * uart, size_t rx_level)
{
    /* Assert RTS if level is half */
    uart_set_rts(uart, rx_level < uart->rx_buf.max_size/2);
}

static void uart_rx_interrupt(int n)
{
    uart_t * uart = uarts[n];

    if(uart == NULL)
    {
        switch(n)
        {
        case 0: UCSR0B &= ~_BV(RXCIE0); break;
        case 1: UCSR1B &= ~_BV(RXCIE1); break;
        case 2: UCSR2B &= ~_BV(RXCIE2); break;
        case 3: UCSR3B &= ~_BV(RXCIE3); break;
        default:
            abort();
        }
    }

    /* Read status before reading UDR clears it */
    uint8_t status = *uart->ucsra;

    /* Read the UDR no matter what */
    uint8_t b = *uart->udr;

    if(status & _BV(FE0))
    {
        uart->frame_error += 1;
    }

    if(status & _BV(DOR0))
    {
        uart->rx_overflow += 1;
    }

    if(status & _BV(UPE0))
    {
        uart->parity_error += 1;
    }

    size_t level;
    int ret = buf_push_byte(&uart->rx_buf, b, &level);
    if(ret < 0)
    {
        uart->rx_overflow += 1;
    }

    /* Assert RTS if level is half */
    uart_update_rts(uart, level);
}

void uart_tx_interrupt(int n)
{
    uart_t * uart = uarts[n];

    if(uart == NULL)
    {
        switch(n)
        {
        case 0: UCSR0B &= ~_BV(UDRIE0); break;
        case 1: UCSR1B &= ~_BV(UDRIE1); break;
        case 2: UCSR2B &= ~_BV(UDRIE2); break;
        case 3: UCSR3B &= ~_BV(UDRIE3); break;
        default:
            abort();
        }
    }

    /* Stop transmitting if not clear to send */
    if(!uart_cts(uart))
    {
        *uart->ucsrb &= ~_BV(UDRIE0);
    }

    size_t level;
    int ret = buf_pop_byte(&uart->tx_buf, &level);

    if(ret < 0)
    {
        /* No more data for now, stop transmitting */
        *uart->ucsrb &= ~_BV(UDRIE0);
    }
    else
    {
        *uart->udr = (uint8_t)ret;
    }
}

ISR(USART0_RX_vect)     { uart_rx_interrupt(0); }
ISR(USART1_RX_vect)     { uart_rx_interrupt(1); }
ISR(USART2_RX_vect)     { uart_rx_interrupt(2); }
ISR(USART3_RX_vect)     { uart_rx_interrupt(3); }

ISR(USART0_UDRE_vect)   { uart_tx_interrupt(0); }
ISR(USART1_UDRE_vect)   { uart_tx_interrupt(1); }
ISR(USART2_UDRE_vect)   { uart_tx_interrupt(2); }
ISR(USART3_UDRE_vect)   { uart_tx_interrupt(3); }

void uart_enable(uart_t * uart)
{
    /* Disable things to make sure everything is stopped */
    uart_disable(uart);

    size_t rx_level = uart->rx_buf.size;
    uart_update_rts(uart, rx_level);

    uarts[uart->num] = uart;
    *uart->ucsrb |= _BV(RXEN0) | _BV(TXEN0) | _BV(RXCIE0) | _BV(UDRIE0);
}

void uart_disable(uart_t * uart)
{
    /* Stop traffic until UART is enabled */
    uart_set_rts(uart, false);

    /* Stop interrupts */
    *uart->ucsrb &= ~(_BV(RXCIE0) | _BV(TXCIE0) | _BV(UDRIE0));
    uarts[uart->num] = NULL;
}

void uart_service(uart_t * uart)
{
    /* Run TX interrupt service to check if we are CTS (or do nothing!) */
    *uart->ucsrb |= _BV(UDRIE0);
}

int uart_recv_byte(uart_t * uart)
{
    size_t level;
    int ret = buf_pop_byte(&uart->rx_buf, &level);
    uart_update_rts(uart, level);

    return ret;
}

int uart_send_byte(uart_t * uart, uint8_t b)
{
    size_t level;
    volatile int ret = buf_push_byte(&uart->tx_buf, b, &level);

    /* Re-enable interrupt to handle potential TX hardware flow stalls or
     * if we ran out of data */
    *uart->ucsrb |= _BV(UDRIE0);

    return ret;
}
