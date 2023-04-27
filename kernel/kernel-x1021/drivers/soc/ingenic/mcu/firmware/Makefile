##############################  Handle IRQ  #############################
HAVE_I2C = y
#HAVE_DMA = y
#HAVE_UART = y
#HAVE_GPIO = y

CHIP_TYPE = x1000
CUR_DIR = $(PWD)

##############################     TOOLS    #############################
CROSS_COMPILE := mips-linux-gnu-
CC = $(CROSS_COMPILE)gcc
LD = $(CROSS_COMPILE)ld
OBJDUMP = $(CROSS_COMPILE)objdump
OBJCOPY = $(CROSS_COMPILE)objcopy

drop-sections := .reginfo .mdebug .oomment .note .pdr .options .MIPS.options
strip-flags   := $(addprefix --remove-section=,$(drop-sections))

HEX_CFLAGS += -Iinclude
HEX_CFLAGS += -Wall -Wundef -Werror-implicit-function-declaration \
	 -fno-common  -Os -march=mips32 -mabi=32 -G 0 -mno-abicalls -fno-pic \
	 -fno-builtin-puts -fno-builtin-putc
HEX_CFLAGS += -I$(CUR_DIR)/include -I$(CUR_DIR)/include/mach
HEX_CFLAGS += -I$(CUR_DIR)/../../../../../arch/mips/xburst/soc-$(CHIP_TYPE)/include

HEX_LDFLAGS	:= -nostdlib -T target.ld
HEX_OBJCOPY_ARGS := -O elf32-tradlittlemips

HEX_NAME := firmware_mcu.hex

OBJS := src/start.o		\
	src/main.o		\
	src/traps.o		\
	src/timer.o		\
	src/bitops.o	\
	src/debug.o

ifeq ($(HAVE_I2C), y)
OBJS += src/i2c.o
HEX_CFLAGS += -DHANDLE_I2C
endif
ifeq ($(HAVE_DMA), y)
OBJS += src/dma.o
HEX_CFLAGS += -DHANDLE_DMA
endif
ifeq ($(HAVE_UART), y)
OBJS += src/uart.o
HEX_CFLAGS += -DHANDLE_UART
endif
ifeq ($(HAVE_GPIO), y)
OBJS += src/gpio.o
HEX_CFLAGS += -DHANDLE_GPIO
endif

all: firmware.bin
	@hexdump -v -e '"0x" 1/4 "%08x" "," "\n"' $< > $(HEX_NAME)

firmware.bin:firmware.o
	@$(LD) -nostdlib -T target.ld -EL $(OBJS) -Map tmp.map -o tmp.elf
	@$(OBJDUMP) -D tmp.elf > tmp.dump
	@$(OBJCOPY) $(strip-flags) $(HEX_OBJCOPY_ARGS) -O binary tmp.elf $@

firmware.o : $(OBJS)

%.o:%.c
	@$(CC) $(HEX_CFLAGS) -o $@ -c $^

%.o:%.S
	@$(CC) $(HEX_CFLAGS) -o $@ -c $^

clean:
	@find . -name "*.o" | xargs rm -f
	@find . -name "*.o.cmd" | xargs rm -f
	@find . -name "*.hex" | xargs rm -f
	@find . -name "*.bin" | xargs rm -f
	@rm -f tmp.map tmp.elf tmp.dump
