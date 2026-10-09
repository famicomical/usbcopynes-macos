# macOS command-line build. Needs Homebrew libftdi: brew install libftdi
CMDS   = dumpcart makenes makeunif splitnes splitunif playcart runplug writewram nesinfo fixgar ramcart playlog disasm breakbank
SRCS   = main.c shim.c io_libftdi.c ../misc.c ../config.c ../crc32.c $(CMDS:%=../c_%.c)
TARGET = ../../bin/copynes

CFLAGS  = -O2 -Wall -Wno-unused-variable -Wno-unused-but-set-variable -Wno-parentheses -Wno-unused-function -Wno-extra-tokens -Wno-pointer-sign -fcommon -include StdAfx.h -I. $(shell pkg-config --cflags libftdi1)
LDLIBS  = $(shell pkg-config --libs libftdi1)

$(TARGET): $(SRCS) StdAfx.h
	$(CC) $(CFLAGS) -o $@ $(SRCS) $(LDLIBS)

clean:
	rm -f $(TARGET)

.PHONY: clean
