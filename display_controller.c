#include "display_controller.h"

#include <stdlib.h>
#include <string.h>

void disp_init(display_t * d)
{
    memset(d, 0, sizeof(*d));

    d->mode = COLOR_BLINK;
    d->color[0] = 0x00;
    d->color[1] = 0x10;
    d->color[2] = 0x10;
    d->color[3] = 0;
    swtimer_set(&d->t, 1000000);

    d->mode = RAINBOW;
    d->frameAdvance = 20;
    d->pixelAdvance = 5;
    swtimer_set(&d->t, 16000);

    neo_drive_init(&d->d, 80, &PORTD, &DDRD, _BV(3));

    d->light = true;
    memset(d->pix_buf, 0x00, sizeof(d->pix_buf));
	for(size_t i = 0; i < NUM_LED; ++i)
	{
        d->pix_buf[i*PIXEL_SIZE(PIX)+0] = d->light ? d->color[0] : 0x00;
        d->pix_buf[i*PIXEL_SIZE(PIX)+1] = d->light ? d->color[1] : 0x00;
        d->pix_buf[i*PIXEL_SIZE(PIX)+2] = d->light ? d->color[2] : 0x00;
        d->pix_buf[i*PIXEL_SIZE(PIX)+3] = d->light ? d->color[3] : 0x00;
	}
    neo_drive_start_show(&d->d, sizeof(d->pix_buf), d->pix_buf);
    while(neo_drive_service(&d->d) != NEO_COMPLETE) {}

}

static void color_blink_service(display_t * d)
{
    if(swtimer_is_expired(&d->t))
    {
        swtimer_set(&d->t, 1000000);
        memset(d->pix_buf, 0x00, sizeof(d->pix_buf));
        for(size_t i = 0; i < NUM_LED; ++i)
        {
            d->pix_buf[i*PIXEL_SIZE(PIX)+0] = d->light ? d->color[0] : 0x00;
            d->pix_buf[i*PIXEL_SIZE(PIX)+1] = d->light ? d->color[1] : 0x00;
            d->pix_buf[i*PIXEL_SIZE(PIX)+2] = d->light ? d->color[2] : 0x00;
            d->pix_buf[i*PIXEL_SIZE(PIX)+3] = d->light ? d->color[3] : 0x00;
        }
        d->light = !d->light;

        neo_drive_start_show(&d->d, sizeof(d->pix_buf), d->pix_buf);
        while(neo_drive_service(&d->d) != NEO_COMPLETE) {}
    }
}

static void set_color(display_t * d, size_t pix, uint8_t r, uint8_t g, uint8_t b)
{
    d->pix_buf[pix*PIXEL_SIZE(PIX)+0] = g;
    d->pix_buf[pix*PIXEL_SIZE(PIX)+1] = r;
    d->pix_buf[pix*PIXEL_SIZE(PIX)+2] = b;
    d->pix_buf[pix*PIXEL_SIZE(PIX)+3] = 0;
}

static void rainbowCycle(display_t * d) {
    if(!swtimer_is_expired(&d->t)) return;
    swtimer_restart(&d->t, 16000);

    // Hue is a number between 0 and 3*256 than defines a mix of r->g->b where
    // hue of 0 = Full red
    // hue of 128 = 1/2 red and 1/2 green
    // hue of 256 = Full Green
    // hue of 384 = 1/2 green and 1/2 blue
    // ...

    unsigned int currentPixelHue = d->firstPixelHue;

    for(unsigned int i=0; i< NUM_LED; i++) {
        if (currentPixelHue>=(3*256)) {                  // Normalize back down incase we incremented and overflowed
            currentPixelHue -= (3*256);
        }

        unsigned char phase = currentPixelHue >> 8;
        unsigned char step = currentPixelHue & 0xff;
         
        switch (phase) {
        case 0: 
            set_color(d, i, ~step, step, 0);
            break;
        case 1: 
            set_color(d, i, 0, ~step, step);
            break;
        case 2: 
            set_color(d, i, step, 0, ~step);
            break;
        }

        currentPixelHue += d->pixelAdvance;                                      
    } 

    d->firstPixelHue += d->frameAdvance;

    neo_drive_start_show(&d->d, sizeof(d->pix_buf), d->pix_buf);
    while(neo_drive_service(&d->d) != NEO_COMPLETE) {}
}

void disp_service(display_t * d)
{
    switch(d->mode)
    {
    case COLOR_BLINK:
        color_blink_service(d);
        break;
    case RAINBOW:
        rainbowCycle(d);
        break;
    }

}

