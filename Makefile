PROG=_mental_fsm.bin


BUILD_DIR=./obj
CCP=g++
CCC=gcc
CFLAGS=-Wall -O0 -c -g3 
CF_TCL_H= `pkg-config --cflags libxml-2.0` \
          `pkg-config --cflags fann` \
          `pkg-config --cflags libpcre`
          

LDFLAGS= -lpthread \
         -ltcl8.5 \
         `pkg-config --libs libxml-2.0` \
         `pkg-config --libs fann` \
         `pkg-config --libs libpcre`

#SRCS=   main.cpp \
#		mentalstate.cpp \
#		mental_fsm.cpp \
#		time_trigger.cpp \
#		log_trigger.cpp \
#		xml_config.cpp \
#		trigger_tree.cpp \
#		virt_trigger.cpp \
#		net_link.cpp \
#		base_trigger.cpp \
#		neuro_trigger.cpp \
#		neuro_state.cpp \
#		neuro_union.cpp \
#		common.cpp \
#		pcre_trigger.cpp \
#		pcre_pattern.cpp \
#		pcre_queue.cpp \
#		crc.cpp \
#		back_messages.cpp

SRCS=$(wildcard *.cpp)

OBJS:=$(SRCS:%.cpp=$(BUILD_DIR)/%.o)

all:$(BUILD_DIR)/$(PROG)

$(BUILD_DIR)/$(PROG): dirs $(OBJS)
	@echo [LD] $(PROG); \
	$(CCP) $(OBJS) $(LDFLAGS) -o $(BUILD_DIR)/$(PROG)
	@cp $(BUILD_DIR)/$(PROG) ../bin/
	@echo Done

dirs: FORCE
	-@if [ ! -d $(BUILD_DIR) ]; then mkdir $(BUILD_DIR); fi
	@cp -R ./bin ../

$(BUILD_DIR)/%.o: %.cpp
	@echo [CC] $< ; \
	$(CCP) $(CFLAGS) $(CF_TCL_H) -MD -o $@ -c $< ;


include $(wildcard $(BUILD_DIR)/*.d) 

clean:
	rm -rf $(BUILD_DIR)
	
	
FORCE: