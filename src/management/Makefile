LIB=libmanage.a

LIB_OBJS= macro_supervisor.o

CFLAGS += -I $(TOP)/src/utils
CFLAGS += -I $(TOP)/src/basic
CFLAGS += -I $(TOP)/src/macro
CFLAGS += -I /root/boost/include

include ../lib.rules

prepare : $(LIB)

$(LIB) : $(LIB_OBJS)
	@echo "  AR " $@
	@$(AR) crT $@ $^
