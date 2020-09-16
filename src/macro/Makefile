LIB=libmacro.a

LIB_OBJS=macro.o platform/$(PLATFORM)/macro_$(PLATFORM).o

CFLAGS += -I $(TOP)/src/utils
CFLAGS += -I $(TOP)/src/basic
CFLAGS += -I $(TOP)/src/event
CFLAGS += -I $(TOP)/src/macro
CFLAGS += -I $(TOP)/src/parse
ifeq ($(PLATFORM), windows)
	CFLAGS += -I $(TOP)/src/event/platform/windows
endif

include ../lib.rules

prepare : $(LIB)

$(LIB) : $(LIB_OBJS)
	@echo "  AR " $@
	@$(AR) crT $@ $^ $(TOP)/src/basic/libbasic.a
