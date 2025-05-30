item= macro management
LIB_PATH=$(shell pwd)/../src/

CFLAGS += -I $(TOP)/src/utils
CFLAGS += -I $(TOP)/src/basic
CFLAGS += -I $(TOP)/src/event
CFLAGS += -I $(TOP)/src/macro
CFLAGS += -I $(TOP)/src/management

CFLAGS += -I /root/source/opencv_backup/opencv-windows/include/opencv4
DLFLAGS += -Wl,-Bstatic -lstdc++ -lpthread -Wl,-Bdynamic
DLFLAGS += -L/root/source/opencv/build-windows/lib -L/root/source/opencv/build-windows/3rdparty/lib
DLFLAGS += -L/root/source/zlib_bin/lib
OPENCV_LIB_REF = -lopencv_imgcodecs4120 -lopencv_highgui4120 -lopencv_imgproc4120 -lopencv_core4120
OPENCV_3RD_LIB_REF = -llibwebp -llibtiff -llibjpeg-turbo -llibpng -llibopenjp2 -lIlmImf -lz
ifeq ($(PLATFORM), windows)
	CFLAGS += -I $(TOP)/src/event/platform/windows
	SUFFIX = .exe
endif

.PHONY: macro
.PHONY: management
.PHONY: macro_flask

all: $(item)
	mv $(addsuffix .exe, $?) $(UNIT_TEST_BIN_PATH)

macro: macro.o
	@printf '  %-10s  %s\n' "BUILD" $@
	@$(CXX) $(CFLAGS) -o $@$(SUFFIX) $< -L $(LIB_PATH)event -levent \
		-L $(LIB_PATH)macro -lmacro -L $(LIB_PATH)utils -lutils -lgdi32 \
		-L $(LIB_PATH)parse -l parse $(DLFLAGS) $(OPENCV_LIB_REF) $(OPENCV_3RD_LIB_REF) 

management: management.o
	@printf '  %-10s  %s\n' "BUILD" $@
	@$(CXX) $(CFLAGS) -o $@$(SUFFIX) $< -L $(LIB_PATH)event -levent \
		-L $(LIB_PATH)parse -l parse -L $(LIB_PATH)management -l manage \
		-L $(LIB_PATH)macro -lmacro -L $(LIB_PATH)utils -lutils -lgdi32 \
		$(DLFLAGS) $(OPENCV_LIB_REF) $(OPENCV_3RD_LIB_REF)

macro_flask: macro_flask.o
	@printf '  %-10s  %s\n' "BUILD" $@
	@$(CXX) $(CFLAGS) -o $@$(SUFFIX) $< -L $(LIB_PATH)event -levent -L $(LIB_PATH)macro -lmacro \
		-L $(LIB_PATH)parse -l parse $(DLFLAGS)

image_recognize_test: image_recognize_test.o
	@printf '  %-10s  %s\n' "BUILD" $@
	@$(CXX) $(CFLAGS) -o $@$(SUFFIX) $< -L $(LIB_PATH)event -levent -L $(LIB_PATH)macro -lmacro \
		-L $(LIB_PATH)parse -l parse $(DLFLAGS) -L/root/source/opencv/build-windows/lib -lopencv_imgcodecs4120 -lopencv_highgui4120 -lopencv_imgproc4120 -lopencv_core4120 -L/root/source/zlib_bin/lib -L/root/source/opencv/build-windows/3rdparty/lib -llibwebp -llibtiff -llibjpeg-turbo -llibpng -llibopenjp2 -lIlmImf -lz -lwinpthread

%.o: %.cpp
	@printf '  %-10s  %s\n' "CXX" $@
	@$(CXX) -c -o $@ $(CFLAGS) $<

clean:
	rm -f *~ *.o *.d *.gcno *.gcda *.gconv $(addsuffix .exe, $(item))
