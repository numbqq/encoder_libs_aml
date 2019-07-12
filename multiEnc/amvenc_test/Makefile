# Make file to  for  encoder library for Linux platform
#
#
#--------------------------------------
.PHONY: CREATE_DIR clean all

CROSS_CC_PREFIX = aarch64-linux-gnu-

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

CFLAGS += -fPIC -g

LDFLAGS =

#CFLAGS += -static
#CLFAGS += -Wno-implicit-function-declaration -Wno-format -Wl,--fatal-warning
#CLFAGS += -Wall -Werror

INC = -I./include -I. -I../vpuapi/include -I../vpuapi


CFLAGS  +=  $(INC) $(DEFINES) $(PLATFORM_FLAGS)
ARFLAGS += rcs
TARGET = aml_enc_test

LDLIBS += -L.
LDLIBS += -lm -lrt -lamvenc_api -lvpcodec 
# pthread 
LDLIBS += -lpthread


#LDFLAGS += -shared

OBJDIR=obj
ALLOBJS=*.o
ALLDEPS=*.dep
ALLLIBS=*.a
RM=rm -f
MKDIR=mkdir -p

SOURCES = test.c test_dma.c
OBJECNAMES = test.o test_dma.o

#OBJECTNAMES=$(patsubst %.cpp,%.o, $(SOURCES))
#OBJECTPATHS=$(addprefix $(OBJDIR)/,$(notdir $(OBJECTNAMES)))

#$(info $$(OBJECTNAMES) is [${OBJECTNAMES}])

#$(info $$(OBJECTPATHS) is [${OBJECTPATHS}])


$(TARGET): $(OBJECNAMES)
	$(LINKER) -o $(TARGET) $(LDFLAGS) $(LDLIBS) $(OBJECNAMES)
	-cp $(TARGET) ../


$(OBJECNAMES):%.o:%.c
	$(LINKER) $(LDFLAGS) -c $< -o $@

-include $(OBJECTPATHS:.o=.dep)

clean: 
	$(RM) $(TARGET)
	$(RM) $(OBJDIR)/$(ALLOBJS)
	$(RM) $(OBJDIR)/$(ALLDEPS)
	$(RM) *.o
	$(RM) *.so

CREATE_DIR:
	-mkdir -p $(OBJDIR)

