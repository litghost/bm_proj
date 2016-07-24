XBEE_LIB = ../xbee_lib

HEADERS = \
	uart.h \
	circ_buf.h \
	neopixel_drive.h \
	swtimer.h \
	$(XBEE_LIB)/xbee.h \

SRCS = \
	main.c \
	uart.c \
	circ_buf.c \
	neopixel_drive.c \
	swtimer.c \
	$(XBEE_LIB)/xbee.c \

bm_proj.elf: $(HEADERS) $(SRCS)
	avr-gcc -I$(XBEE_LIB) -g -Os -std=gnu99 -mmcu=atmega2560 $(SRCS) -DF_CPU=16000000UL -Wall -Werror -o bm_proj.elf -Wl,--defsym,boot_program_page=0x000000000003e488
	avr-size bm_proj.elf

.PHONY: size
size: bm_proj.elf
	avr-size bm_proj.elf


.PHONY: load
load: bm_proj.elf
	avrdude -p m2560 -c wiring -P /dev/ttyACM0 -b 115200 -F -D -U flash:w:bm_proj.elf -v

.PHONY: clean
clean:
	rm bm_proj.elf
