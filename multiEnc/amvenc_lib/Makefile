# Make file to  for  encoder library for Linux platform
#
#
#--------------------------------------
.PHONY: CREATE_DIR clean all

CROSS_CC_PREFIX = aarch64-linux-gnu-

CC  = $(CROSS_CC_PREFIX)gcc
CXX = $(CROSS_CC_PREFIX)g++
LINKER=$(CC)
AR  = $(CROSS_CC_PREFIX)ar

CFLAGS += -O2 -std=c99
CFLAGS+= -fPIC -g

LDFLAGS =

#CFLAGS += -static
#CLFAGS += -Wno-implicit-function-declaration -Wno-format -Wl,--fatal-warning
#CLFAGS += -Wall -Werror

INC = -I./include -I. -I../vpuapi/include -I../vpuapi -I../test_sample/helper/


CFLAGS  +=  $(INC) $(DEFINES) $(PLATFORM_FLAGS)
ARFLAGS += rcs
TARGET = libvpcodec.so

LDLIBS += -lm -lrt 
# pthread 
LDLIBS += -lpthread 
LIB=libamvenc_api.so


LDFLAGS += -shared

OBJDIR=obj
ALLOBJS=*.o
ALLDEPS=*.dep
ALLLIBS=*.a
RM=rm -f
MKDIR=mkdir -p

SOURCES = libvpmulti_codec.c  \
		  AML_MultiEncoder.c \

OBJECTNAMES=$(patsubst %.c,%.o, $(SOURCES))
OBJECTPATHS=$(addprefix $(OBJDIR)/,$(notdir $(OBJECTNAMES)))

$(info $$(OBJECTNAMES) is [${OBJECTNAMES}])

$(info $$(OBJECTPATHS) is [${OBJECTPATHS}])


all: CREATE_DIR $(OBJECTPATHS)
	$(LINKER) -o $(TARGET) $(LDFLAGS) $(LIB) -Wl,-gc-section -Wl,--start-group $(OBJECTPATHS) $(LDLIBS) -Wl,--end-group
	-cp libvpcodec.so ../amvenc_test

-include $(OBJECTPATHS:.o=.dep)

clean: 
	$(RM) $(TARGET)
	$(RM) $(OBJDIR)/$(ALLOBJS)
	$(RM) $(OBJDIR)/$(ALLDEPS)
	$(RM) *.o
	$(RM) *.so

CREATE_DIR:
	-mkdir -p $(OBJDIR)

obj/%.o: %.c
	$(CC) $(CFLAGS) -Wall -c $< -o $@ -MD -MF $(@:.o=.dep)
