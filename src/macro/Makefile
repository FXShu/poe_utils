LIB=libmacro.a

LIB_OBJS=macro.o macro_factory.o platform/$(PLATFORM)/macro_$(PLATFORM).o

CFLAGS += -I $(TOP)/src/utils
CFLAGS += -I $(TOP)/src/basic
CFLAGS += -I $(TOP)/src/event
CFLAGS += -I $(TOP)/src/macro
CFLAGS += -I $(TOP)/src/parse
ifeq ($(PLATFORM), windows)
	CFLAGS += -I $(TOP)/src/event/platform/windows
	CFLAGS += -I $(TOP)/src/basic/platform/windows
endif

include ../lib.rules

prepare : $(LIB)

$(LIB) : $(LIB_OBJS)
	@printf '  %-10s  %s\n' "AR" $@
	@$(AR) crT $@ $^
