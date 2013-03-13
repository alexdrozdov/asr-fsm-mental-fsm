CCP=g++
CCC=gcc
PROTOC=/home/adrozdov/dev/speech/aux-packages/build/bin/protoc

PROG=fann_train.bin
BUILD_DIR=./obj

CFLAGS=-Wall -O0 -g3  -I /home/adrozdov/dev/speech/aux-packages/build/include/ -c 
LDFLAGS=-lpthread -ldl  -lprotobuf -L/home/adrozdov/dev/speech/aux-packages/build/lib/ -lpcre -lfloatfann -ltcl8.5

SRCS=$(wildcard *.cpp)
OBJS:=$(SRCS:%.cpp=$(BUILD_DIR)/%.o)

PROTO=$(wildcard *.proto)
PROTO_CC=$(PROTO:%.proto=%.pb.cc)
PROTO_OBJ=$(PROTO_CC:%.cc=./obj/%.o)

all:$(BUILD_DIR)/$(PROG) install.gnulinux

$(BUILD_DIR)/$(PROG): dirs $(PROTO_CC) $(PROTO_OBJ) $(OBJS)
	@echo [LD] $(PROG); \
	$(CCP) $(OBJS) $(PROTO_OBJ) $(LDFLAGS) -o $(BUILD_DIR)/$(PROG)

install.gnulinux: FORCE
	@cp $(BUILD_DIR)/$(PROG) ../../bin/
	@echo Done
	
install.macos: FORCE
	@./inst_name_tool.sh $(BUILD_DIR)/$(PROG) libprotobuf.7.dylib  libs/libprotobuf.7.dylib
	@./inst_name_tool.sh $(BUILD_DIR)/$(PROG) libfloatfann.2.dylib libs/libfloatfann.2.dylib
	@./inst_name_tool.sh $(BUILD_DIR)/$(PROG) libpcre.0.dylib      libs/libpcre.0.dylib
	@cp $(BUILD_DIR)/$(PROG) ../../bin/
	@echo Done

dirs: FORCE
	-@if [ ! -d $(BUILD_DIR) ]; then mkdir $(BUILD_DIR); fi
	-@if [ ! -d ../../bin ]; then mkdir ../../bin; fi

$(BUILD_DIR)/%.o: %.cpp
	@echo [CC] $< ; \
	$(CCP) $(CFLAGS) $(CFLAGS_AUX) -MD -o $@ -c $< ;

$(BUILD_DIR)/%.o: %.cc
	@echo [CC] $< ; \
	$(CCP) $(CFLAGS) $(CFLAGS_AUX) -MD -o $@ -c $< ;
	
%.pb.cc: %.proto
	@echo [PROTO] $< ; \
	$(PROTOC) -I=./ --cpp_out=./ $< ;

include $(wildcard $(BUILD_DIR)/*.d) 

clean:
	rm -rf $(BUILD_DIR) *.pb.cc *.pb.h
	
FORCE:
