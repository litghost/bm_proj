#include <stdbool.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <string.h>

#include "uart.h"
#include "swtimer.h"
#include "neopixel_drive.h"

static uart_t u0;
static uint8_t tx0_buf[32];
static uint8_t rx0_buf[32];

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

uint8_t pix_buf[BUF_SIZE(8, WS2812_RBG)];

int main(void)
{
    stdout = &mystdout;

    swtimer_setup();

    uart_init(&u0, 0, 
        sizeof(rx0_buf), rx0_buf,
        sizeof(tx0_buf), tx0_buf);
    uart_set_baudrate(&u0, 19200);
    uart_enable(&u0);

    neo_drive_t d;
    neo_drive_init(&d, 80, &PORTD, &DDRD, _BV(3));

    sei();

    swtimer_t t;
    swtimer_set(&t, 100);

    uint8_t x = 0;
    while(true)
    {
        memset(pix_buf, x, sizeof(pix_buf));
        neo_drive_start_show(&d, sizeof(pix_buf), pix_buf);
        while(neo_drive_service(&d) != NEO_COMPLETE) {}

        while(!swtimer_is_expired(&t)) {}
        swtimer_set(&t, 1000);

        x += 1;
    }
}
