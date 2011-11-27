CCP=g++
CCC=gcc
CFLAGS=-Wall -c -g
CF_TCL_H=-I ../tcl8.5.9/generic/ -I ../libxml2-2.7.8/include -I ../fann-2.1.0/src/include
LDFLAGS=-L ../tcl8.5.9/unix/ -ltcl8.5 -L ../libxml2-2.7.8/.libs/ -L ../fann-2.1.0/src/.libs -lxml2 -lpthread -lfann

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

PROG=mental_fsm.bin
BUILD_DIR=./obj

all:$(BUILD_DIR)/$(PROG)

$(BUILD_DIR)/$(PROG): dirs $(OBJS)
	@echo [LD] $(PROG); \
	$(CCP) $(OBJS) $(LDFLAGS) -o $(BUILD_DIR)/$(PROG)
	@cp $(BUILD_DIR)/$(PROG) ../bin/
	@cp ../tcl8.5.9/unix/libtcl8.5.so ../bin/
	@cp ./mental_fsm.sh ../bin/
	@cp ../libxml2-2.7.8/.libs/libxml2.so.2.7.8 ../bin/
	@rm -f ../bin/libxml2.so
	@ln -s ../bin/libxml2.so.2.7.8 ../bin/libxml2.so
	@echo Done

dirs: FORCE
	-@if [ ! -d $(BUILD_DIR) ]; then mkdir $(BUILD_DIR); fi

$(BUILD_DIR)/%.o: %.cpp
	@echo [CC] $< ; \
	$(CCP) $(CFLAGS) $(CF_TCL_H) -o $@ -c $< ;


clean:
	rm -rf $(BUILD_DIR)
	
	
FORCE: