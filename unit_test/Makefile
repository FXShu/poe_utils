item= macro management macro_flask
LIB_PATH=$(shell pwd)/../src/
all: $(item)

CFLAGS += -I $(TOP)/src/utils
CFLAGS += -I $(TOP)/src/basic
CFLAGS += -I $(TOP)/src/event
CFLAGS += -I $(TOP)/src/macro
CFLAGS += -I $(TOP)/src/management

DLFLAGS += -Wl,-Bstatic -lstdc++ -lpthread -Wl,-Bdynamic

ifeq ($(PLATFORM), windows)
	CFLAGS += -I $(TOP)/src/event/platform/windows
	SUFFIX = .exe
endif

.PHONY: macro
.PHONY: management
.PHONY: macro_flask

macro: macro.o
	@echo "  BUILD  " $@
	@$(CXX) $(CFLAGS) -o $@$(SUFFIX) $< -L $(LIB_PATH)event -levent -L $(LIB_PATH)macro -lmacro \
		-L $(LIB_PATH)parse -l parse $(DLFLAGS)

management: management.o
	@echo "  BUILD  " $@
	@$(CXX) $(CFLAGS) -o $@$(SUFFIX) $< -L $(LIB_PATH)event -levent -L $(LIB_PATH)macro -lmacro \
		-L $(LIB_PATH)parse -l parse -L $(LIB_PATH)management -l manage $(DLFLAGS)

macro_flask: macro_flask.o
	@echo "  BUILD  " $@
	@$(CXX) $(CFLAGS) -o $@$(SUFFIX) $< -L $(LIB_PATH)event -levent -L $(LIB_PATH)macro -lmacro \
		-L $(LIB_PATH)parse -l parse $(DLFLAGS)

%.o: %.cpp
	@echo "  CXX " $<
	@$(CXX) -c -o $@ $(CFLAGS) $<

clean:
	rm -f *~ *.o *.d *.gcno *.gcda *.gconv $(addsuffix .exe, $(item))
