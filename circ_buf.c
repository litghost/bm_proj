#include "circ_buf.h"
#include <util/atomic.h>
#include <string.h>
#include <assert.h>

void buf_init(volatile circ_buf_t * b, size_t buf_size, void * buf)
{
    b->data = buf;
    b->size = 0;
    b->idx = 0;
    b->max_size = buf_size;
}

int buf_push_byte(volatile circ_buf_t * b, uint8_t c, size_t * level)
{
    int ret;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        if(b->size >= b->max_size)
        {
            *level = b->size;
            ret = -1;
        }
        else
        {
            size_t idx = b->idx + b->size;
            if(idx >= b->max_size)
            {
                idx -= b->max_size;
            }

            b->size += 1;
            b->data[idx] = c;
            *level = b->size;
            ret = 0;
        }
    }

    return ret;
}

int buf_pop_byte(volatile circ_buf_t * b, size_t * level)
{
    int ret;

    ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
    {
        if(b->size == 0)
        {
            *level = 0;
            ret = -1;
        }
        else
        {
            ret = b->data[b->idx];

            b->idx += 1;
            b->size -= 1;
            if(b->idx >= b->max_size)
            {
                b->idx = 0;
            }

            *level = b->size;
        }
    }

    return ret;
}
