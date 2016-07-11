#ifndef _NEOPIXEL_DRIVE_H_
#define _NEOPIXEL_DRIVE_H_

#include "uart.h"
#include "swtimer.h"

typedef uint32_t pixel_format_t;

#define PIX_FORMAT(R, G, B, W) \
        ((R << 0) | (G << 8) | (B << 16) | (W << 24))

#define NO_PIX 0xFF

#define WS2812_RBG PIX_FORMAT(8, 0, 16, 0xFF)

#define R_OFFSET(t) ((t >>  0) & 0xFF)
#define G_OFFSET(t) ((t >>  8) & 0xFF)
#define B_OFFSET(t) ((t >> 16) & 0xFF)
#define W_OFFSET(t) ((t >> 24) & 0xFF)

#define PIXEL_SIZE(t) ( \
    (R_OFFSET(t) != NO_PIX) + \
    (G_OFFSET(t) != NO_PIX) + \
    (B_OFFSET(t) != NO_PIX) + \
    (W_OFFSET(t) != NO_PIX) + \
    0)

#define BUF_SIZE(n, t) (n*PIXEL_SIZE(t))

#define LINE_BITS_PER_BIT 4 /* It takes 4 bits on the line to represent 1 bit */
#define MAX_PIXEL_SIZE (4*8) /* RGBW (4) at 8-bits per pixel */

typedef enum {
    NEO_IDLE,
    NEO_WAITING,
    NEO_COMPLETE,
} neo_drive_status_t;

typedef struct
{
    uart_t * uart;

    size_t buf_size;
    uint8_t * buf;

    volatile uint8_t * port;
    volatile uint8_t * ddr;
    uint8_t pinmask;

    bool show_pending;
    swtimer_t timer;
    uint8_t reset_period; /* Number of microseconds to wait before next cycle */
} neo_drive_t;

void neo_drive_init(neo_drive_t * d, uart_t * uart, uint8_t reset_period, volatile uint8_t * port, volatile uint8_t * ddr, uint8_t pinmask);

/*! Starts a drive if not already started */
void neo_drive_start_show(neo_drive_t * d, size_t buf_size, void * buf);

/*! Get status of drive FSM */
neo_drive_status_t neo_drive_service(neo_drive_t * d); 


#endif /* _NEOPIXEL_DRIVE_H_ */
