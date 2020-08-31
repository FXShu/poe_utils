CROSS=i686-w64-mingw32-
CXX=$(CROSS)g++

CFLAGS= -g -O0 -Wall -static-libgcc -static-libstdc++

BIN_PATH=$(shell pwd)/bin/
SRC_PATH=$(shell pwd)/src/
TOP=$(shell pwd)
BIN=poed

include .config

ifeq ($(CONFIG_PLATFORM), WINDOWS)
PLATFORM=windows
else
PLATFORM=linux
endif

export CXX CFLAGS TOP PLATFORM

all:prepare $(BIN)

prepare:
ifeq "$(wildcard $(BIN_PATH))" ""
	@mkdir $(BIN_PATH)
endif
	$(MAKE) -C $(SRC_PATH) $@

$(BIN) : 

clean:
	@rm -f *~ *.o *.d *.gcno *.gcda *.gconv $(all)
	$(MAKE) -C src $@
