LIB=libparse.a

LIB_OBJS=parser.o
CFLAGS += -I $(TOP)/src/utils
include ../lib.rules

prepare: $(LIB)

$(LIB) : $(LIB_OBJS)
	@echo "  AR  " $@
	@$(AR) crT $@ $^
