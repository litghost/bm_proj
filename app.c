#include <stdbool.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include <string.h>
#include <util/crc16.h>
#include <util/delay.h>

#include "uart.h"
#include "swtimer.h"
#include "xbee.h"
#include "wireless_protocol.h"
#include "display_controller.h"

static uart_t u0;
static uint8_t tx0_buf[32];
static uint8_t rx0_buf[32];

static uart_t u3;
static uint8_t tx3_buf[256];
static uint8_t rx3_buf[128];

static display_t d;

static xbee_interface_t xbee;
static xbee_uart_interface_t xbee_uart;
static uint8_t xbee_buf[256];
static uint8_t frame[128];
static xbee_parsed_frame_t parsed_frame;

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

static int xbee_uart_write(void * ptr, const void * buf, size_t n)
{
    uart_t * u = (uart_t*)ptr;
    const uint8_t * b = buf;

    int sent = n;
    for(size_t i = 0; i < n; ++i)
    {
        int ret = uart_send_byte(u, b[i]);
        if(ret < 0)
        {
            sent = i;
            break;
        }
    }

    return sent;
} 

int xbee_uart_read(void * ptr, void *buf, size_t nbyte)
{
    uart_t * u = (uart_t*)ptr;
    uint8_t * b = buf;

    size_t recv = nbyte;
    for(size_t i = 0; i < nbyte; ++i)
    {
        int ret = uart_recv_byte(u);
        if(ret < 0)
        {
            recv = i;
            break;
        }

        b[i] = ret;
    }

    return recv;

}

unsigned xbee_sleep(unsigned sec)
{
    uint32_t expire = swtimer_now_msec() + sec*1000;
    while(swtimer_now_msec() < expire) {};

    return 0;
}

static void send_reply(xbee_interface_t * xbee, xbee_parsed_frame_t *parsed_frame, const char * reply)
{
    xbee_address_t addr;
    if(parsed_frame->api_id == XBEE_RECEIVE)
    {
        addr.type = XBEE_64_BIT;
        addr.addr.address = parsed_frame->frame.receive.responder_address;
    }
    else
    {
        addr.type = XBEE_16_BIT;
        addr.addr.network_address = parsed_frame->frame.receive.responder_network_address;
    }

    size_t reply_size = strlen(reply);
    int ret = xbee_transmit(xbee, 5, &addr, 0,  reply_size, reply);
    if(ret != 0)
    {
        printf("Failed to send reply, ret = %d!\n", ret);
    }
}

#define REPLY(msg) send_reply(xbee, parsed_frame, msg "\r\n");

extern int start_boot(void) __attribute__((noreturn));

static void handle_packet(xbee_interface_t * xbee, xbee_parsed_frame_t *parsed_frame)
{
    size_t packet_size = parsed_frame->frame.receive.packet_size;
    const uint8_t * packet_data = parsed_frame->frame.receive.packet_data;

    if(packet_size == 0)
    {
        REPLY("hi");
    }

    switch(packet_data[0])
    {
    case OP_NOP:
        REPLY("app");
        break;
    case OP_JUMP_TO_APP:
        REPLY("ok"); 
        break;
    case OP_JUMP_TO_STAGE0:
        REPLY("ok"); 
        while(buf_get_level(&u3.tx_buf) > 0) { _delay_ms(1); }
        _delay_ms(100);
        cli();
        start_boot();
        break;
    default:
        REPLY("unknown op");

    }
}

int main(void)
{
    stdout = &mystdout;

    swtimer_setup();

    uart_init(&u0, 0, 
        sizeof(rx0_buf), rx0_buf,
        sizeof(tx0_buf), tx0_buf);
    uart_set_baudrate(&u0, 19200);
    uart_enable(&u0);

    uart_init(&u3, 3, 
        sizeof(rx3_buf), rx3_buf,
        sizeof(tx3_buf), tx3_buf);
    uart_set_baudrate(&u3, 9600);
    uart_set_hardware_flow(&u3, E, 3, G, 5);
    uart_enable(&u3);

    sei();

    xbee_uart.ptr = &u3;
    xbee_uart.write = xbee_uart_write;
    xbee_uart.read = xbee_uart_read;
    xbee_uart.sleep = xbee_sleep;

    disp_init(&d);

    while(1)
    {
        PORTH &= ~_BV(3);
        DDRH |= _BV(3);

        swtimer_t t;
        swtimer_set(&t, 1000000);
        while(swtimer_is_expired(&t) == 0) {};

        PORTH &= ~_BV(3);
        DDRH &= ~_BV(3);

        swtimer_set(&t, 1000000);
        while(swtimer_is_expired(&t) == 0) {};

        int ret = xbee_open(&xbee, &xbee_uart, sizeof(xbee_buf), xbee_buf);
        if(ret < 0)
        {
            printf("Failed to init xbee! %d\n", ret);
            continue;
        }
        break;
    }

    while(true) {
        uart_service(&u0);
        uart_service(&u3);

        disp_service(&d);

        memset(frame, 0, sizeof(frame));
        int ret = xbee_recv_frame(&xbee, sizeof(frame), frame);
        if(ret > 0)
        {
            
            int parse = xbee_parse_frame(&parsed_frame, ret, frame);
            if(parse != 0)
            {
                printf("Parse error %d, frame len %d api id %d\n", parse, ret, frame[0]);
            }

            switch(parsed_frame.api_id)
            {
            case XBEE_RECEIVE_16_BIT:
            case XBEE_RECEIVE:
                handle_packet(&xbee, &parsed_frame);
                break;
            case XBEE_TRANSMIT_STATUS:
                break;
            default:
                /* Do nothing */
                break;
            }
        }
    }
}

