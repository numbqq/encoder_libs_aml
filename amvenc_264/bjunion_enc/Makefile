#export CROSS_CC_PREFIX=aarch64-linux-gnu-
#export CC=$(CROSS_CC_PREFIX)gcc
#export CXX=$(CROSS_CC_PREFIX)g++
#export AR=$(CROSS_CC_PREFIX)ar

CFLAGS +=-O2 -fPIC -mstrict-align
CFLAGS += -g
#LDLIBS +=  -L./h264bitstream/.libs -lh264bitstream -lm -lrt -shared
LDLIBS += -lh264bitstream -lm -lrt -shared

SUFFIXES += .d
NODEPS := clean distclean

C_SRCS := $(wildcard *.c) $(wildcard enc/gx_enc_fast/*.c)

C_OBJS := $(C_SRCS:%.c=%.o)

CXX_SRCS := $(wildcard *.cpp) $(wildcard enc/gx_enc_fast/*.cpp)
CXX_OBJS := $(CXX_SRCS:%.cpp=%.o)

DEPFILES := $(patsubst %c,%d,$(C_SRCS))
DEPFILES += $(patsubst %cpp,%d,$(CXX_SRCS))


INC = -I./include -I./enc/gx_enc_fast -I. -Ih264bitstream
TARGET = libamvenc_264.so

all: $(TARGET)

ifeq "$(findstring $(MAKECMDGOALS),$(NODEPS))"  ""
  -include $(DEPFILES)
endif


$(TARGET): $(C_OBJS) $(CXX_OBJS)
	@echo " [LINK] $@"
	$(CXX) $(LDLIBS) -Wl,-soname,$(TARGET) -o $@ $^

%.o: %.c
	@echo " [CXX] $<"
	@$(CC) -c $(CFLAGS) $(INC) $< -o $@

%.o: %.cpp
	@echo " [CXX] $<"
	@$(CXX) -c $(CFLAGS) $(INC) $< -o $@

%.d: %.cpp
	@echo " [DEP] $<"
	@$(CXX) $(CFLAGS) $(INC) -MM -MT "$(patsubst %.cpp,%.o,$<)" $< -MF $@

%.d: %.c
	@echo " [DEP] $<"
	@$(CC) $(CFLAGS) -MM -MT "$(patsubst %.c,%.o,$<)" $< -MF $@

clean:
	@rm -f $(DEPFILES)
	@rm -f $(C_OBJS) $(CXX_OBJS)
	@rm -f $(TARGET)
