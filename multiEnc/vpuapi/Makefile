# Make file to  for  encoder library for Linux platform
#
#
#--------------------------------------
.PHONY: CREATE_DIR clean all

CROSS_CC_PREFIX = arm-linux-gnueabihf-

ifeq ($(ARM), 0)
CROSS_CC_PREFIX = aarch64-linux-gnu-
endif

CC  = $(CROSS_CC_PREFIX)gcc
CXX = $(CROSS_CC_PREFIX)g++
LINKER=$(CC)
AR  = $(CROSS_CC_PREFIX)ar

ifeq ($(ARM), 0)
CFLAGS += -O2 -std=c99
else
CFLAGS += -DARM
endif

CFLAGS+= -fPIC -g

LDFLAGS =

#CFLAGS += -static
#CLFAGS += -Wno-implicit-function-declaration -Wno-format -Wl,--fatal-warning
#CLFAGS += -Wall -Werror

INC = -I./include -I./wave -I.


CFLAGS  +=  $(INC) $(DEFINES) $(PLATFORM_FLAGS)
ARFLAGS += rcs

TARGET = ../libamvenc_api.so
LDLIBS += -lm -lrt
# pthread 
LDLIBS += -lpthread 
LDFLAGS += -shared

INC = -I. -I./include -I./wave 
	
TARGET= ../libamvenc_api.so
OBJDIR=obj
ALLOBJS=*.o
ALLDEPS=*.dep
ALLLIBS=*.a
RM=rm -f
MKDIR=mkdir -p

SOURCES = product.c \
		  vdi.c \
		  vdi_osal.c \
          vpuapifunc.c \
          vpuapi.c \
          wave/wave5.c \
		  debug.c

VPATH = .:
VPATH += wave

OBJECTNAMES=$(patsubst %.c,%.o, $(SOURCES))
OBJECTPATHS=$(addprefix $(OBJDIR)/,$(notdir $(OBJECTNAMES)))


$(info $$(OBJECTNAMES) is [${OBJECTNAMES}])

$(info $$(OBJECTPATHS) is [${OBJECTPATHS}])


all: CREATE_DIR $(OBJECTPATHS)
	$(LINKER) -o $(TARGET) $(LDFLAGS) -Wl,-gc-section -Wl,--start-group $(OBJECTPATHS) $(LDLIBS) -Wl,--end-group

-include $(OBJECTPATHS:.o=.dep)

clean: 
	$(RM) $(TARGET)
	$(RM) $(OBJDIR)/$(ALLOBJS)
	$(RM) $(OBJDIR)/$(ALLDEPS)

CREATE_DIR:
	-mkdir -p $(OBJDIR)

obj/%.o: %.c
	$(CC) $(CFLAGS) -Wall -c $< -o $@ -MD -MF $(@:.o=.dep)

