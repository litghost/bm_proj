#ifndef _VECTOR_MANAGER_H_
#define _VECTOR_MANAGER_H_

#define VECTORS(X) \
	X(TIMER1_COMPA_vect) \
	X(USART0_RX_vect) \
	X(USART1_RX_vect) \
	X(USART2_RX_vect) \
	X(USART3_RX_vect) \
	X(USART0_UDRE_vect) \
	X(USART1_UDRE_vect) \
	X(USART2_UDRE_vect) \
	X(USART3_UDRE_vect) \

typedef enum
{
#define VECTOR_ENUM(n) \
	VECT_ ## n, 
	VECTORS(VECTOR_ENUM)
	NUM_VECTORS
} vector_t;

typedef void (*vector_fun_t)(void); 

void vector_clear_all(void);
void vector_set_vector(vector_t v, vector_fun_t f);

#endif /* _VECTOR_MANAGER_H_ */
