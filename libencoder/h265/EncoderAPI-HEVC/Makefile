CXX:=aarch64-linux-gnu-g++
TARGET=h265EncoderDemo
AMLENC_LIB=test.o
LDFLAGS += -lm -lrt -lge2d -lion -Lhevc_enc -lvphevcodec
#LDFLAGS += -lm -lrt  -Lhevc_enc -lvphevcodec
CFLAGS+=-O2 -g -I./../../../aml_libion/include/ion
LIBS += -L./../../../aml_libion/ -L./../../../aml_libge2d/libge2d

$(TARGET):$(AMLENC_LIB)
	$(CXX) $(CFLAGS) $(LDFLAGS) $(LIBS) $< -o $(TARGET) $(LinkIn)

$(AMLENC_LIB):%.o:%.cpp
	$(CXX) -c $(CFLAGS) $< -o $@
clean:
	-rm -f *.o
	-rm -f $(TARGET)
