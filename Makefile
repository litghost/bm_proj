XBEE_LIB = xbee_lib

HEADERS = \
	uart.h \
	circ_buf.h \
	neopixel_drive.h \
	swtimer.h \
	vector_manager.h \
	display_controller.h \
	$(XBEE_LIB)/xbee.h \

SRCS = \
	uart.c \
	circ_buf.c \
	neopixel_drive.c \
	swtimer.c \
	vector_manager.c \
	display_controller.c \
	$(XBEE_LIB)/xbee.c \

DEFINES = \
	-D__ASSERT_USE_STDERR \

all: bm_proj.elf bm_proj_app.elf

APP_START = 0x10000
BOOT_PROGRAM_LOC = 0x3e488

bm_proj.elf: $(HEADERS) $(SRCS) main.c vector_manager_extern.c
	avr-gcc -I$(XBEE_LIB) -g -Os -std=gnu99 -mmcu=atmega2560 main.c vector_manager_extern.c $(SRCS) $(DEFINES) -DF_CPU=16000000UL -Wall -Werror -o bm_proj.elf -Wl,--defsym,boot_program_page=$(BOOT_PROGRAM_LOC) -Wl,--defsym,start_app=$(APP_START) -Wl,-Map,bm_proj.map
	avr-size bm_proj.elf

bm_proj_app.elf: $(HEADERS) $(SRCS) app.c bm_proj.elf
	avr-gcc -I$(XBEE_LIB) -g -Os -std=gnu99 -mmcu=atmega2560 $(SRCS) $(DEFINES) app.c -DF_CPU=16000000UL -Wall -Werror -o bm_proj_app.elf -Wl,--defsym,start_boot=0x00000 -Wl,--section-start=.text=$(APP_START) -Wl,-Map,bm_proj_app.map -Wl,--defsym,vect_list=0x$(shell avr-objdump bm_proj.elf -t | grep vect_list | cut -d ' ' -f1)
	avr-size bm_proj_app.elf

bm_proj_app.bin: bm_proj_app.elf
	avr-objcopy -O binary bm_proj_app.elf bm_proj_app.bin

.PHONY: app_load
app_load: bm_proj_app.bin
	python program_proj.py --write-app bm_proj_app.bin

.PHONY: size
size: bm_proj.elf
	avr-size bm_proj.elf


.PHONY: load
load: bm_proj.elf
	avrdude -p m2560 -c wiring -P /dev/ttyACM0 -b 115200 -F -D -U flash:w:bm_proj.elf -v

.PHONY: clean
clean:
	rm -f *.elf *.map *.bin *.pyc
	make -C wireless_bootloader clean
