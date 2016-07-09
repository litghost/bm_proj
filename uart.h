#ifndef _UART_H_
#define _UART_H_

#include <stdint.h>
#include <stdlib.h>
#include "circ_buf.h"
#include <avr/io.h>

typedef struct
{
    int num;

    volatile circ_buf_t tx_buf;
    volatile circ_buf_t rx_buf;

    volatile int frame_error;
    volatile int rx_overflow;
    volatile int parity_error;

    volatile uint8_t * udr; 
    volatile uint8_t * ucsra; 
    volatile uint8_t * ucsrb; 
    volatile uint8_t * ucsrc; 
    volatile uint8_t * ubrrl; 
    volatile uint8_t * ubrrh; 

    uint8_t ipin_cts;
    volatile uint8_t * pin_cts; /* Clear to send, okay to send more bytes */

    uint8_t iport_rts;
    volatile uint8_t * port_rts; /* Request to send, please send me more byte */
} uart_t;

#define uart_init(ptr, n, _rx_size, _rx_buf, _tx_size, _tx_buf) \
    _uart_init(ptr, n, _rx_size, _rx_buf, _tx_size, _tx_buf, \
        &UDR ## n, \
        &UCSR ## n ## A, \
        &UCSR ## n ## B, \
        &UCSR ## n ## C, \
        &UBRR ## n ## L, \
        &UBRR ## n ## H) \

void _uart_init(uart_t * uart, 
        int uart_num,
        size_t rx_buf_size, void * rx_buf, 
        size_t tx_buf_size, void * tx_buf,
        volatile uint8_t * udr, 
        volatile uint8_t * ucsra, volatile uint8_t * ucsrb, volatile uint8_t * ucsrc,
        volatile uint8_t * ubrrl, volatile uint8_t * ubrrh);

#define uart_set_hardware_flow(ptr, cts_port, cts_pin, rts_port, rts_pin) \
    _uart_set_hardware_flow(ptr, cts_pin, \
            &PIN ## cts_port, &DDR ## cts_port, &PORT ## cts_port, \
            rts_pin, &PORT ## rts_port, &DDR ## rts_port)

#define uart_clear_hardware_flow(ptr) \
    _uart_set_hardware_flow(ptr, 0, NULL, NULL, NULL, 0, NULL, NULL)

void _uart_set_hardware_flow(uart_t * uart, 
    uint8_t ipin_cts, volatile uint8_t * pin_cts, 
    volatile uint8_t * ddr_cts, volatile uint8_t * port_cts,
    uint8_t iport_rts, volatile uint8_t * port_rts, volatile uint8_t * ddr_rts);

int uart_set_baudrate(uart_t * uart, long baudrate);

void uart_enable(uart_t * uart);
void uart_disable(uart_t * uart);
void uart_service(uart_t * uart);

int uart_recv_byte(uart_t * uart);
int uart_send_byte(uart_t * uart, uint8_t b);

#endif /* _UART_H_ */
