
SRCFILES=efuse.c

all: clean efuse

efuse: $(SRCFILES)
	mips-linux-gnu-gcc -o $@ $(SRCFILES)
	mips-linux-gnu-strip $@
clean:
	rm -rf efuse
