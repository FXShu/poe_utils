LIB=libmacro.a

LIB_OBJS=macro.o macro_factory.o platform/$(PLATFORM)/macro_$(PLATFORM).o

CFLAGS += -I $(TOP)/src/utils
CFLAGS += -I $(TOP)/src/basic
CFLAGS += -I $(TOP)/src/event
CFLAGS += -I $(TOP)/src/macro
CFLAGS += -I $(TOP)/src/parse
CFLAGS += -I $(TOP)/3rd_party/leptonica-win32/include
CFLAGS += -I $(TOP)/3rd_party/openssl-win32/include
CFLAGS += -I $(TOP)/3rd_party/tesseract-win32/include
CFLAGS += -DDPP_STATIC
CFLAGS += -I /root/dpp-win32/include

ifeq ($(PLATFORM), windows)
	CFLAGS += -I $(TOP)/src/event/platform/windows
	CFLAGS += -I $(TOP)/src/basic/platform/windows
endif

include ../lib.rules

prepare : $(LIB)

$(LIB) : $(LIB_OBJS)
	@printf '  %-10s  %s\n' "AR" $@
	@$(AR) crT $@ $^
