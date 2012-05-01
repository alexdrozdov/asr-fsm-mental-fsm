CCP=g++
CCC=gcc
CFLAGS=-Wall -c -g -fPIC `pkg-config --cflags libxml-2.0`
LDFLAGS=-shared `pkg-config --libs libxml-2.0`

SRCS=   xml_config.cpp

OBJS:=$(SRCS:%.cpp=./obj/%.o)

OS= $(shell uname -s)
ifeq ("$(OS)","Linux")
	LIB_EXT=so
else
	ifeq ("$(OS)","Darwin")
	        LIB_EXT=dylib
	endif
endif

PROG=libxmlsup.$(LIB_EXT)
BUILD_DIR=./obj

all:$(BUILD_DIR)/$(PROG)

$(BUILD_DIR)/$(PROG): dirs $(OBJS)
	@echo [LD] $(PROG); \
	$(CCP) $(OBJS) $(LDFLAGS) -o $(BUILD_DIR)/$(PROG)
	@cp $(BUILD_DIR)/$(PROG) ../../bin/libs/

$(BUILD_DIR)/%.o: %.cpp
	@echo [CC] $< ; \
	$(CCP) $(CFLAGS) $(CF_TCL_H) -o $@ -c $< ;
	
dirs: FORCE
	-@if [ ! -d $(BUILD_DIR) ]; then mkdir $(BUILD_DIR); fi

	
FORCE:

clean:
	rm -rf $(BUILD_DIR)