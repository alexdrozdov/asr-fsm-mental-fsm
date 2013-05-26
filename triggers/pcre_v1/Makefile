CCP=g++
CCC=gcc
CFLAGS=-Wall -c -g -fPIC

OS=$(shell uname -s)
ifeq ($(OS),Darwin)
	CF_AUX_FLAGS= `pkg-config --cflags libxml-2.0` \
          -I ../../../../aux-packages/pcre-8.12/ \
          -I ../../xml_support/ \
          -I ../../../../aux-packages/tcl8.5.9/generic/ \
          -I ../../../../aux-packages/build/include/ \
          -I ../../../mental-fsm/p2vera/source/ \
          -DMACOSX -DAUXPACKAGES
    LIB_EXT=dylib
    install_targets=install.macos
    
    LDFLAGS= -shared \
         -lpthread \
         -ltcl8.5 \
         `pkg-config --libs libxml-2.0` \
         `pkg-config --libs libpcre` \
         -L ../../../bin/libs/ -lp2vera \
         -lxmlsup -L ../../xml_support/obj/
else
ifeq ($(AUXBUILD),AUX)
	CF_AUX_FLAGS= `pkg-config --cflags libxml-2.0` \
          -I ../../xml_support/ \
          -I ../../../../aux-packages/tcl8.5.9/generic/ \
          -I ../../../../aux-packages/build/include/ \
          -I ../../../mental-fsm/p2vera/source/ \
          -DGNULINUX -DAUXPACKAGES
          
	LDFLAGS= -shared \
         -lpthread \
         -ltcl8.5 \
         `pkg-config --libs libxml-2.0` \
         `pkg-config --libs libpcre` \
         -L ../../../bin/libs/ -lp2vera \
         -lxmlsup -L ../../xml_support/obj/
else
    CF_AUX_FLAGS= `pkg-config --cflags libxml-2.0` \
          `pkg-config --cflags libpcre` \
          -I ../../../mental-fsm/p2vera/source/ \
          -I ../../xml_support/ \
          -DGNULINUX
          
    LDFLAGS= -shared \
         -lpthread \
         -ltcl8.5 \
         `pkg-config --libs libxml-2.0` \
         `pkg-config --libs libpcre` \
         -L ../../../bin/libs/ -lp2vera \
         -lxmlsup -L ../../xml_support/obj/
endif
	LIB_EXT=so
    install_targets=install.gnulinux
endif          
          
CF_FSM_FLAGS=-I ../../

SRCS=$(wildcard *.cpp)
OBJS:=$(SRCS:%.cpp=./obj/%.o)

PROG=libpcre_trigger_v1.$(LIB_EXT)
BUILD_DIR=./obj

all:$(BUILD_DIR)/$(PROG) $(install_targets)

$(BUILD_DIR)/$(PROG): dirs $(OBJS)
	@echo [LD] $(PROG); \
	$(CCP) $(OBJS) $(LDFLAGS) -o $(BUILD_DIR)/$(PROG)

$(BUILD_DIR)/%.o: %.cpp
	@echo [CC] $< ; \
	$(CCP) $(CFLAGS) $(CF_AUX_FLAGS) $(CF_FSM_FLAGS) -MD -o $@ -c $< ;

install.gnulinux: FORCE
	@cp $(BUILD_DIR)/$(PROG) ../../../bin/libs/
	
install.macos: FORCE
	@install_name_tool -change ./obj/libxmlsup.dylib  @executable_path/libs/libxmlsup.dylib $(BUILD_DIR)/$(PROG)
	@install_name_tool -change ./obj/libp2vera.dylib  @executable_path/libs/libp2vera.dylib $(BUILD_DIR)/$(PROG)
	@cp $(BUILD_DIR)/$(PROG) ../../../bin/libs/


dirs: FORCE
	-@if [ ! -d $(BUILD_DIR) ]; then mkdir $(BUILD_DIR); fi


include $(wildcard $(BUILD_DIR)/*.d) 
	
FORCE:

clean:
	rm -rf $(BUILD_DIR)