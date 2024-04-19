TARGET=h265EncoderDemo
SOURCES=test.cpp test_dma.c
AMLENC_LIB=$(patsubst %.c, %.o, $(patsubst %.cpp, %.o, $(SOURCES)))
LDFLAGS += -lm -lrt -Lhevc_enc -lamvenc_265 -lge2d -lion -lpthread
CFLAGS +=-O2 -g -mstrict-align
INC = -I./hevc_enc/include/
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
	make -C hevc_enc/ clean
