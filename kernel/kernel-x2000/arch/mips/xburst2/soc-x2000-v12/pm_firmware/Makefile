
CHIP_TYPE ?= x2000
current_dir = $(PWD)

OBJS=src/start.o src/regs_save_restore.o src/pm.o src/pm_sleep.o src/pm_fastboot.o src/printf.o src/uart.o

LD_FLAGS=-static -n -nostdlib -EL -m elf32ltsmip -T pm_firmware.lds --gc-sections -pie -Bstatic -Ttext 0

CFLAGS := -fPIC  -O2 -mips32r2 -fvisibility=hidden  -fno-common -fno-builtin -nostdlib
CFLAGS +=  -I$(current_dir)/include
CFLAGS +=  -DDEBUG


PRJ=pm_firmware
BIN=$(PRJ).bin
HEX=$(PRJ).hex
OBJCFLAGS = --gap-fill=0xff


all:$(HEX)

$(HEX):$(BIN)
	@hexdump -v -e '"0x" 1/4 "%08x" "," "\n"' $< > $@
$(BIN):$(PRJ)
	mips-linux-gnu-objcopy --gap-fill=0xff -O binary $< $@

$(PRJ):$(OBJS)
	mips-linux-gnu-ld -o $(PRJ) $(LD_FLAGS) $^ -Map $(PRJ).map
	mips-linux-gnu-objdump $(PRJ) -D > $(PRJ).dump

src/start.o:src/start.S
	mips-linux-gnu-gcc  $(CFLAGS)  -o $@ -c $^
src/regs_save_restore.o:src/regs_save_restore.S
	mips-linux-gnu-gcc  $(CFLAGS)  -o $@ -c $^

%.o:%.c
	mips-linux-gnu-gcc  $(CFLAGS)  -o $@ -c $^
clean:
	rm $(BIN) $(PRJ) $(OBJS) $(PRJ).map $(HEX) -rf
