CROSS=i686-w64-mingw32-
CXX=$(CROSS)g++

CFLAGS= -g -std=c++11 -O0 -Wall -static-libgcc -static-libstdc++

BIN_PATH=$(shell pwd)/bin/
UNIT_TEST_BIN_PATH=$(BIN_PATH)unit_test
SRC_PATH=$(shell pwd)/src/
TOP=$(shell pwd)
BIN=poed

include .config

ifeq ($(CONFIG_PLATFORM), WINDOWS)
PLATFORM=windows
else
PLATFORM=linux
endif

export CXX CFLAGS TOP PLATFORM UNIT_TEST_BIN_PATH

all:prepare unit

prepare:
ifeq "$(wildcard $(BIN_PATH))" ""
	@mkdir $(BIN_PATH)
endif
	@$(MAKE) -C $(SRC_PATH) $@

clean:
	@rm -f *~ *.o *.d *.gcno *.gcda *.gconv $(all)
	$(MAKE) -C src $@
	$(MAKE) -C unit_test $@
	@rm -fr $(BIN_PATH)

unit: prepare
	@mkdir -p $(UNIT_TEST_BIN_PATH)
	@$(MAKE) -C unit_test
