#ifndef _CIRC_BUF_H_
#define _CIRC_BUF_H_

#include <stdint.h>
#include <stdlib.h>

typedef struct
{
    size_t idx;
    size_t size;
    size_t max_size;
    uint8_t * data;
} circ_buf_t;

void buf_init(volatile circ_buf_t * b, size_t buf_size, void * buf);

/*! Push a byte into the buffer
 *
 * \return 0 for success, <0 otherwise */
int buf_push_byte(volatile circ_buf_t * b, uint8_t c, size_t * level);

/*! Pops a byte of the buffer 
 *
 * \return Return 0-UINT8_MAX if pop works, <0 otherwise */
int buf_pop_byte(volatile circ_buf_t * b, size_t * level);

#endif /* _CIRC_BUF_H_ */
