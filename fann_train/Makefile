CCP=g++
CCC=gcc
CFLAGS=-Wall -c -g
CFLAGS_AUXLIB=-I ../../tcl8.5.9/generic/ \
              -I ../../fann-2.1.0/src/include

LDFLAGS_AUXLIBS=-L ../../fann-2.1.0/src/.libs  -lfann \
        -L ../../tcl8.5.9/unix/ -ltcl8.5 -ltclreadline

SRCS=   main.cpp \
        common.cpp \
        opt_handlers.cpp \
        packet_funcs.cpp \
        interactive.cpp

OBJS:=$(SRCS:%.cpp=./obj/%.o)

PROG=fann_train.bin
BUILD_DIR=./obj

all:$(BUILD_DIR)/$(PROG)

$(BUILD_DIR)/$(PROG): dirs $(OBJS)
	@echo [LD] $(PROG); \
	$(CCP) $(OBJS) $(LDFLAGS_AUXLIBS) $(LDFLAGS) -o $(BUILD_DIR)/$(PROG)
	@cp $(BUILD_DIR)/$(PROG) ../../bin/
	@echo Done

dirs: FORCE
	-@if [ ! -d $(BUILD_DIR) ]; then mkdir $(BUILD_DIR); fi

$(BUILD_DIR)/%.o: %.cpp
	@echo [CC] $< ; \
	$(CCP) $(CFLAGS_AUXLIB) $(CFLAGS) -o $@ -c $< ;


FORCE:

clean:
	rm -rf $(BUILD_DIR)

