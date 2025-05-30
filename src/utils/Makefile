LIB=libutils.a

LIB_OBJS= platform/$(PLATFORM)/utils_$(PLATFORM).o
CFLAGS += -I $(TOP)/src/utils
include ../lib.rules

prepare : $(LIB)

$(LIB) : $(LIB_OBJS)
	@printf '  %-10s  %s\n' "AR" $@
	@$(AR) crT $@ $^ $(TOP)/src/basic/libbasic.a
