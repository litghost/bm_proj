#ifndef _DISPLAY_CONTROLLER_H_
#define _DISPLAY_CONTROLLER_H_

#include "params.h"
#include "swtimer.h"
#include "neopixel_drive.h"
#include <stdbool.h>

typedef enum {
    NUM_DISP_EVENTS,
} disp_event_t;

typedef enum {
    COLOR_BLINK,
    RAINBOW,
    SINGLE,
    SOLID,
} disp_mode_t;

typedef struct {
    disp_mode_t mode;

    uint8_t pix_buf[BUF_SIZE(NUM_LED, SK6812RGBW)];
    neo_drive_t d;

    swtimer_t t;
    
    /* Blink vars */
    uint8_t color[4];
    bool light;

    /* Rainbow vars */
    unsigned int firstPixelHue; // Color for the first pixel in the string
    unsigned int frameAdvance;
    unsigned int pixelAdvance;

    uint16_t idx;
} display_t;

void disp_init(display_t * d);
void disp_service(display_t * d);
void disp_set_one(display_t * d, uint16_t idx);

#endif /* _DISPLAY_CONTROLLER_H_ */
