#include "uart.h"
#include <stdbool.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include "swtimer.h"

static uart_t u0;
static uint8_t tx_buf[32];
static uint8_t rx_buf[32];

int uart0_put(char c, FILE *f)
{
    if(c == '\n')
    {
        while(uart_send_byte(&u0, '\r') < 0) {}
    }
	while(uart_send_byte(&u0, c) < 0) {}

    return 0;
}

static FILE mystdout = FDEV_SETUP_STREAM(uart0_put, NULL, _FDEV_SETUP_WRITE);

int main(void)
{
    uart_init(&u0, 0, 
        sizeof(rx_buf), rx_buf,
        sizeof(tx_buf), tx_buf);
    uart_set_baudrate(&u0, 19200);
    uart_enable(&u0);
    swtimer_setup();

    sei();

	fprintf(&mystdout, "hello world!\n");
	//printf("hello world!\n");

    swtimer_t t;
    swtimer_set(&t, 1000000);

    while(true)
    {
        if(swtimer_is_expired(&t))
        {
            fprintf(&mystdout, "It is now %lu\n", swtimer_now_usec());
            swtimer_set(&t, 1000000);
        }

        int ret = uart_recv_byte(&u0);
        if(ret >= 0)
        {
            if(u0.rx_overflow)
            {
                uart_send_byte(&u0, 'o');
            }

            if(u0.frame_error)
            {
                uart_send_byte(&u0, 'f');
            }
            uart_send_byte(&u0, ret);
        }
    }
}
