LIB=libbasic.a

LIB_OBJS=observer.o

CFLAGS += -I $(TOP)/src/utils
include ../lib.rules

prepare : $(LIB)

$(LIB) : $(LIB_OBJS)
	@printf '  %-10s  %s\n' "AR" $@
	@$(AR) crT $@ $^
