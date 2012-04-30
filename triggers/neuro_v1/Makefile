CCP=g++
CCC=gcc
CFLAGS=-Wall -c -g -fPIC

CF_AUX_FLAGS= `pkg-config --cflags libxml-2.0` \
          `pkg-config --cflags fann`
          
CF_FSM_FLAGS=-I ../../ -I ../../xml_support/

LDFLAGS= -shared \
         -lpthread \
         -ltcl8.5 \
         `pkg-config --libs libxml-2.0` \
         `pkg-config --libs fann` \
         -lxmlsup -L ../../xml_support/obj/ \

SRCS=$(wildcard *.cpp)
OBJS:=$(SRCS:%.cpp=./obj/%.o)

PROG=libneuro_trigger_v1.so
BUILD_DIR=./obj

all:$(BUILD_DIR)/$(PROG)

$(BUILD_DIR)/$(PROG): dirs $(OBJS)
	@echo [LD] $(PROG); \
	$(CCP) $(OBJS) $(LDFLAGS) -o $(BUILD_DIR)/$(PROG)
	@cp $(BUILD_DIR)/$(PROG) ../../../bin/libs/

$(BUILD_DIR)/%.o: %.cpp
	@echo [CC] $< ; \
	$(CCP) $(CFLAGS) $(CF_AUX_FLAGS) $(CF_FSM_FLAGS) -MD -o $@ -c $< ;
	
dirs: FORCE
	-@if [ ! -d $(BUILD_DIR) ]; then mkdir $(BUILD_DIR); fi


include $(wildcard $(BUILD_DIR)/*.d) 
	
FORCE:

clean:
	rm -rf $(BUILD_DIR)