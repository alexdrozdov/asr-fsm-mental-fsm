CFLAGS=-Wall -O0 -c -g3 

OS=$(shell uname -s)
ifeq ($(OS),Darwin)
	CFLAGS_AUX= `pkg-config --cflags libxml-2.0` \
          `pkg-config --cflags libpcre` \
          -I ./xml_support/ \
          -I ../../aux-packages/tcl8.5.9/generic/ \
          -I ../../aux-packages/build/include/ \
          -DMACOSX
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
          -DMACOSX
    install_targets=install.gnulinux
    PROTOC_PATH=$(shell pwd)/../../aux-packages/build/bin/
    
    LDFLAGS=-ltcl8.5 \
        -lpthread \
        -ldl \
        -L ./xml_support/obj/ -lxmlsup  \
        -L ../../aux-packages/build/lib/ -lprotobuf \
        -rdynamic
else
    CFLAGS_AUX= `pkg-config --cflags libxml-2.0` \
          `pkg-config --cflags libpcre` \
          `pkg-config --cflags protobuf` \
          -I ./xml_support/ \
          -DGNULINUX
    install_targets=install.gnulinux
    
    LDFLAGS=-ltcl8.5 \
        -lpthread \
        -ldl \
        -L ./xml_support/obj/ -lxmlsup  \
        `pkg-config --libs protobuf` \
        -rdynamic
endif
endif
     
CCP=g++
CCC=gcc
PROTOC=$(PROTOC_PATH)protoc     

PROG=_mental_fsm.bin
BUILD_DIR=./obj


SRCS=$(wildcard *.cpp)
OBJS:=$(SRCS:%.cpp=$(BUILD_DIR)/%.o)

PROTO=$(wildcard *.proto)
PROTO_CC=$(PROTO:%.proto=%.pb.cc)
PROTO_OBJ=$(PROTO_CC:%.cc=./obj/%.o)

all:$(BUILD_DIR)/$(PROG) log_v1 neuro_v1 pcre_v1 $(install_targets)

$(BUILD_DIR)/$(PROG): dirs xmlsup $(PROTO_CC) $(PROTO_OBJ) $(OBJS)
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
	@cp -R ./bin ../

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

log_v1 : FORCE xmlsup
	@$(MAKE) -C ./triggers/log_v1/ AUXBUILD=$(AUXBUILD)
	
neuro_v1 : FORCE xmlsup
	@$(MAKE) -C ./triggers/neuro_v1/ AUXBUILD=$(AUXBUILD)
	
pcre_v1 : FORCE xmlsup
	@$(MAKE) -C ./triggers/pcre_v1/ AUXBUILD=$(AUXBUILD)
	
xmlsup : FORCE
	@$(MAKE) -C ./xml_support/

clean:
	rm -rf $(BUILD_DIR) *.pb.cc *.pb.h
	@$(MAKE) clean -C ./xml_support/
	@$(MAKE) clean -C ./triggers/log_v1
	@$(MAKE) clean -C ./triggers/neuro_v1
	@$(MAKE) clean -C ./triggers/pcre_v1
	
	
FORCE: