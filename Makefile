PROG=_mental_fsm.bin


BUILD_DIR=./obj
CCP=g++
CCC=gcc
CFLAGS=-Wall -O0 -c -g3 

OS=$(shell uname -s)
ifeq ($(OS),Darwin)
	CFLAGS_AUX= `pkg-config --cflags libxml-2.0` \
          `pkg-config --cflags libpcre` \
          -I ./xml_support/ \
          -I ../../aux-packages/tcl8.5.9/generic/ \
          -DMACOSX
    install_targets=install.macos
else
    CFLAGS_AUX= `pkg-config --cflags libxml-2.0` \
          `pkg-config --cflags libpcre` \
          -I ./xml_support/ \
          -DGNULINUX
    install_targets=install.gnulinux
endif
          

LDFLAGS= -lpthread \
         -ltcl8.5 \
         `pkg-config --libs libxml-2.0` \
         `pkg-config --libs libpcre` \
         -lxmlsup -L ./xml_support/obj \
         -rdynamic

SRCS=$(wildcard *.cpp)

OBJS:=$(SRCS:%.cpp=$(BUILD_DIR)/%.o)

all:$(BUILD_DIR)/$(PROG) log_v1 $(install_targets)

$(BUILD_DIR)/$(PROG): dirs xmlsup $(OBJS)
	@echo [LD] $(PROG); \
	$(CCP) $(OBJS) $(LDFLAGS) -o $(BUILD_DIR)/$(PROG)

install.gnulinux: FORCE
	@cp $(BUILD_DIR)/$(PROG) ../bin/
	@echo Done
	
install.macos: FORCE
	@install_name_tool -change ./obj/libxmlsup.dylib  @executable_path/libs/libxmlsup.dylib $(BUILD_DIR)/$(PROG)
	@cp $(BUILD_DIR)/$(PROG) ../bin/
	@echo Done

dirs: FORCE
	-@if [ ! -d $(BUILD_DIR) ]; then mkdir $(BUILD_DIR); fi
	@cp -R ./bin ../

$(BUILD_DIR)/%.o: %.cpp
	@echo [CC] $< ; \
	$(CCP) $(CFLAGS) $(CFLAGS_AUX) -MD -o $@ -c $< ;


include $(wildcard $(BUILD_DIR)/*.d) 

log_v1 : FORCE xmlsup
	@$(MAKE) -C ./triggers/log_v1/
	
xmlsup : FORCE
	@$(MAKE) -C ./xml_support/

clean:
	rm -rf $(BUILD_DIR)
	@$(MAKE) clean -C ./xml_support/
	@$(MAKE) clean -C ./triggers/log_v1
	
	
FORCE: