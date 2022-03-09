#export CROSS_CC_PREFIX=aarch64-linux-gnu-
#export CC=$(CROSS_CC_PREFIX)gcc
#export CXX=$(CROSS_CC_PREFIX)g++
#export AR=$(CROSS_CC_PREFIX)ar

TARGET=testApi
SOURCES=test.cpp test_dma.c
AMLENC_LIB=$(patsubst %.c, %.o, $(patsubst %.cpp, %.o, $(SOURCES)))
#LDFLAGS += -lm -lrt -Lbjunion_enc -lvpcodec -Lbjunion_enc/h264bitstream -lh264bitstream
LDFLAGS += -lm -lrt -Lbjunion_enc -lvpcodec -lh264bitstream -lion
CFLAGS +=-O2 -g
INC = -I./ -I./bjunion_enc/include/
CFLAGS += $(INC)

$(TARGET):$(AMLENC_LIB)
	$(CXX) $(CFLAGS) $^ -o $(TARGET) $(LDFLAGS)

%.o:%.cpp
	$(CXX) -c $(CFLAGS) $< -o $@
%.o:%.c
	$(CC) -c $(CFLAGS) $< -o $@
clean:
	-rm -f *.o
	-rm -f $(TARGET)
	make -C bjunion_enc/ clean
