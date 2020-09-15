item= macro
LIB_PATH=$(shell pwd)/../src/
all: $(item)

CFLAGS += -I $(TOP)/src/utils
CFLAGS += -I $(TOP)/src/basic
CFLAGS += -I $(TOP)/src/event
CFLAGS += -I $(TOP)/src/macro


ifeq ($(PLATFORM), windows)
	CFLAGS += -I $(TOP)/src/event/platform/windows
endif

macro: macro.o
	@echo "  BUILD  " $@
	@$(CXX) $(CFLAGS) -o $@ $< -L $(LIB_PATH)event -levent -L $(LIB_PATH)macro -lmacro \
		-L $(LIB_PATH)parse -l parse

%.o: %.cpp
	@echo "  CXX " $<
	@$(CXX) -c -o $@ $(CFLAGS) $<

clean:
	rm -f *~ *.o *.d *.gcno *.gcda *.gconv $(addsuffix .exe, $(item))
