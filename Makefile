CCP=g++
CCC=gcc
CFLAGS=-Wall -c -g
CF_TCL_H= `pkg-config --cflags libxml-2.0` \
          `pkg-config --cflags fann` \
          

LDFLAGS= -lpthread \
         -ltcl8.5 \
         `pkg-config --libs libxml-2.0` \
         `pkg-config --libs fann`

SRCS=   main.cpp \
		mentalstate.cpp \
		mental_fsm.cpp \
		time_trigger.cpp \
		log_trigger.cpp \
		xml_config.cpp \
		trigger_tree.cpp \
		virt_trigger.cpp \
		net_link.cpp \
		base_trigger.cpp \
		neuro_trigger.cpp \
		neuro_state.cpp \
		neuro_union.cpp \
		common.cpp

OBJS:=$(SRCS:%.cpp=./obj/%.o)

PROG=_mental_fsm.bin
BUILD_DIR=./obj

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
	$(CCP) $(CFLAGS) $(CF_TCL_H) -o $@ -c $< ;


clean:
	rm -rf $(BUILD_DIR)
	
	
FORCE: