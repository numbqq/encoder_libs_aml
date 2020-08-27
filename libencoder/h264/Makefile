export CROSS_CC_PREFIX=aarch64-linux-gnu-
export CC=$(CROSS_CC_PREFIX)gcc
export CXX=$(CROSS_CC_PREFIX)g++
export AR=$(CROSS_CC_PREFIX)ar

TARGET=testApi
AMLENC_LIB=test.o
LDFLAGS += -lm -lrt -Lbjunion_enc -lvpcodec
CFLAGS +=-O2 -g

$(TARGET):$(AMLENC_LIB)
	$(CXX) $(CFLAGS) $(LDFLAGS) $< -o $(TARGET)

$(AMLENC_LIB):%.o:%.cpp
	$(CXX) -c $(CFLAGS) $< -o $@
clean:
	-rm -f *.o
	-rm -f $(TARGET)
