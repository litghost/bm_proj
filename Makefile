bm_proj.elf: main.c uart.c uart.h circ_buf.c circ_buf.h
	avr-gcc -g -Os -std=gnu99 -mmcu=atmega2560 main.c uart.c circ_buf.c -DF_CPU=16000000 -Wall -Werror -o bm_proj.elf
	avr-size bm_proj.elf

.PHONY: load
load: bm_proj.elf
	avrdude -p m2560 -c wiring -P /dev/ttyACM0 -b 115200 -F -D -U flash:w:bm_proj.elf -v

