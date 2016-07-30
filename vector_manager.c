#include "vector_manager.h"
#include <avr/interrupt.h>
#include <string.h>

extern vector_fun_t vect_list[NUM_VECTORS];

#define VECTOR_FUN(vector) \
void vector (void) __attribute__ ((signal,__INTR_ATTRS)) ; \
void vector (void) { \
	if(vect_list[VECT_ ## vector]) { \
		vect_list[VECT_ ## vector](); \
	} \
}

VECTORS(VECTOR_FUN)

void vector_clear_all(void)
{
	cli();
	memset(vect_list, 0, sizeof(vect_list));
}

void vector_set_vector(vector_t v, vector_fun_t f)
{
	cli();
	vect_list[v] = f;
}
