LIB=libparse.a

LIB_OBJS=parser.o
CFLAGS += -I $(TOP)/src/utils
CFLAGS += -I $(TOP)/src/macro
CFLAGS += -I $(TOP)/src/basic

include ../lib.rules

prepare: $(LIB)

$(LIB) : $(LIB_OBJS)
	@printf '  %-10s  %s\n' "AR" $@
	@$(AR) crT $@ $^
