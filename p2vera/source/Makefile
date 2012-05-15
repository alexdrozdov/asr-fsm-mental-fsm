CFLAGS=-Wall -O0 -c -g3 

OS=$(shell uname -s)
ifeq ($(OS),Darwin)
	CFLAGS_AUX= `pkg-config --cflags libxml-2.0` \
          `pkg-config --cflags libpcre` \
          -I ./xml_support/ \
          -I ../../aux-packages/tcl8.5.9/generic/ \
          -I ../../aux-packages/build/include/ \
          -DMACOSX -DAUXPACKAGES
    install_targets=install.macos
    PROTOC_PATH=$(shell pwd)/../../aux-packages/build/bin/
    
    LDFLAGS=-ltcl8.5 \
        -lpthread \
        -ldl \
        -L ./xml_support/obj/ -lxmlsup  \
        -L ../../aux-packages/build/lib/ -lprotobuf \
        -rdynamic
else
ifeq ($(AUXBUILD),AUX)
	CFLAGS_AUX= `pkg-config --cflags libxml-2.0` \
          -I ../../aux-packages/tcl8.5.9/generic/ \
          -I ../../aux-packages/build/include/ \
          -I ./xml_support/ \
          -DGNULINUX -DAUXPACKAGES
    install_targets=install.gnulinux
    PROTOC_PATH=$(shell pwd)/../../aux-packages/build/bin/
    
    LDFLAGS=-ltcl8.5 \
        -lpthread \
        -ldl \
        -L ./xml_support/obj/ -lxmlsup  \
        -L ../../aux-packages/build/lib/ -lprotobuf \
        -rdynamic
else
    CFLAGS_AUX= \
          -DGNULINUX
    install_targets=install.gnulinux
    
    LDFLAGS= -shared -lpthread -lprotobuf -rdynamic
endif
	LIB_EXT=so
endif

PROG=libp2vera.$(LIB_EXT)
BUILD_DIR=./obj

CCP=g++
CCC=gcc
PROTOC=$(PROTOC_PATH)protoc

SRCS=$(wildcard *.cpp)
OBJS:=$(SRCS:%.cpp=$(BUILD_DIR)/%.o)

PROTO=$(wildcard *.proto)
PROTO_CC=$(PROTO:%.proto=%.pb.cc)
PROTO_OBJ=$(PROTO_CC:%.cc=./obj/%.o)

all:$(BUILD_DIR)/$(PROG) $(install_targets)

$(BUILD_DIR)/$(PROG): dirs $(PROTO_CC) $(PROTO_OBJ) $(OBJS)
	@echo [LD] $(PROG); \
	$(CCP) $(OBJS) $(PROTO_OBJ) $(LDFLAGS) -o $(BUILD_DIR)/$(PROG)

install.gnulinux: FORCE
	@cp $(BUILD_DIR)/$(PROG) ../bin/
	@echo Done
	
install.macos: FORCE
	@install_name_tool -change ./obj/libxmlsup.dylib  @executable_path/libs/libxmlsup.dylib $(BUILD_DIR)/$(PROG)
	@install_name_tool -change /usr/local/lib/libprotobuf.7.dylib  @executable_path/libs/libprotobuf.7.dylib $(BUILD_DIR)/$(PROG)
	@cp $(BUILD_DIR)/$(PROG) ../bin/
	@echo Done

dirs: FORCE
	-@if [ ! -d $(BUILD_DIR) ]; then mkdir $(BUILD_DIR); fi
	-@if [ ! -d ../bin ]; then mkdir ../bin; fi

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
