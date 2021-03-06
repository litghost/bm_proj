#ifndef _WIRELESS_PROTOCOL_H_
#define _WIRELESS_PROTOCOL_H_

typedef enum {
	OP_NOP,
	OP_WRITE_PAGE_BUF,
	OP_CHECK_CRC_PAGE_BUF,
	OP_WRITE_PAGE,
	OP_CHECK_CRC_FLASH,
	OP_JUMP_TO_APP,
	OP_JUMP_TO_STAGE0,
	OP_APP_OP,
} wireless_op_t;

#define BYTES_PER_WRITE_PAGE 64

#endif /* _WIRELESS_PROTOCOL_H_ */
