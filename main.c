#include "uart.h"
#include <stdbool.h>
#include <avr/interrupt.h>

static uart_t u0;
static uint8_t tx_buf[32];
static uint8_t rx_buf[32];

int main(void)
{
    uart_init(&u0, 0, 
        sizeof(rx_buf), rx_buf,
        sizeof(tx_buf), tx_buf);
    uart_set_baudrate(&u0, 9600);
    uart_enable(&u0);

    sei();

    while(true)
    {
        int ret = uart_recv_byte(&u0);
        if(ret >= 0)
        {
            uart_send_byte(&u0, ret);
        }
    }
}
