TARGET=testHevcApi
AMLENC_LIB=test.o
LDFLAGS += -lm -lrt -Lhevc_enc -lvphevcodec -lge2d
CFLAGS+=-O2 -g

$(TARGET):$(AMLENC_LIB)
	$(CXX) $(CFLAGS) $< -o $(TARGET) $(LDFLAGS) $(LinkIn)

$(AMLENC_LIB):%.o:%.cpp
	$(CXX) -c $(CFLAGS) $< -o $@
clean:
	-rm -f *.o
	-rm -f $(TARGET)
	@make -C hevc_enc/ clean
