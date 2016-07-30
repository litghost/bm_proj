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

    swtimer_set(&d->t, 1000000);
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

void disp_service(display_t * d)
{
    switch(d->mode)
    {
    case COLOR_BLINK:
        color_blink_service(d);
        break;
    }

}

