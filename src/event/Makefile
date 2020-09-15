LIB=libevent.a

LIB_OBJS=platform/$(PLATFORM)/interceptor.o
CFLAGS += -I $(TOP)/src/macro
CFLAGS += -I $(TOP)/src/utils
CFLAGS += -I $(TOP)/src/basic
CFLAGS += -I $(TOP)/src/event
CFLAGS += -I $(TOP)/src/parse
include ../lib.rules

prepare: $(LIB)

$(LIB) : $(LIB_OBJS)
	@echo "  AR  " $@
	@$(AR) crT $@ $^
