CCP=g++
CCC=gcc
CFLAGS=-Wall -c -g
CFLAGS_AUXLIB=-I ../../tcl8.5.9/generic/ \
              -I ../../fann-2.1.0/src/include \
              `pkg-config --cflags protobuf` \
              `pcre-config --cflags`

LDFLAGS_AUXLIBS=-L ../../fann-2.1.0/src/.libs  -lfann \
        -L ../../tcl8.5.9/unix/ -ltcl8.5 -ltclreadline \
        `pkg-config --libs protobuf` \
        `pcre-config --libs`

SRCS=   main.cpp \
        common.cpp \
        opt_handlers.cpp \
        packet_funcs.cpp \
        interactive.cpp \
        train_tools.cpp

OBJS:=$(SRCS:%.cpp=./obj/%.o)

PB_SRCS= cfg_serialize.pb.cc
PB_OBJS:=$(PB_SRCS:%.cc=./obj/%.o)

PROG=fann_train.bin
BUILD_DIR=./obj

all:$(BUILD_DIR)/$(PROG)

$(BUILD_DIR)/$(PROG): dirs cfg_serialize.pb.cc $(OBJS) $(PB_OBJS)
	@echo [LD] $(PROG); \
	$(CCP) $(OBJS) $(PB_OBJS) $(LDFLAGS_AUXLIBS) $(LDFLAGS) -o $(BUILD_DIR)/$(PROG)
	@cp $(BUILD_DIR)/$(PROG) ../../bin/
	@echo Done

dirs: FORCE
	-@if [ ! -d $(BUILD_DIR) ]; then mkdir $(BUILD_DIR); fi

$(BUILD_DIR)/%.o: %.cpp
	@echo [CC] $< ; \
	$(CCP) $(CFLAGS_AUXLIB) $(CFLAGS) -o $@ -c $< ;
	
$(BUILD_DIR)/%.o: %.cc
	@echo [CC] $< ; \
	$(CCP) $(CFLAGS_AUXLIB) $(CFLAGS) -o $@ -c $< ;
	
cfg_serialize.pb.cc: cfg_serialize.proto
	@echo [PROTOC] cfg_serialize.proto ; \
	protoc -I=. --cpp_out=. cfg_serialize.proto


FORCE:

clean:
	rm -rf $(BUILD_DIR)

