ifneq ($(buildroot), )
export CC=$(shell find $(buildroot)/host/bin -name '*gnu-gcc' )
export CXX=$(shell find $(buildroot)/host/bin -name '*gnu-g++' )
export AR=$(shell find $(buildroot)/host/bin -name '*gnu-ar' )
export SYSROOT=$(shell find $(buildroot) -name sysroot )
CFLAGS+= --sysroot=$(SYSROOT)

$(warning CC=$(CC))
endif


#export CROSS_CC_PREFIX=aarch64-linux-gnu-
#export CC=${CROSS_CC_PREFIX}gcc
#export CXX=${CROSS_CC_PREFIX}g++
#export AR=${CROSS_CC_PREFIX}ar

TARGET=jpeg_enc_test
LDFLAGS += -lm -lrt  -Ljpeg_enc -ljpegenc_api
CFLAGS+=-g -I./jpeg_enc/include

$(TARGET):test.o test_dma.o
	$(CXX) $(CFLAGS) $^ -o $(TARGET) $(LDFLAGS)

#$(AMLENC_LIB):%.o:%.c
#	$(CXX) -c $(CFLAGS) $< -o $@
.c.o:
	$(CXX) $(CFLAGS) -c $<
clean:
	-rm -f *.o
	-rm -f $(TARGET)
	@make -C jpeg_enc/ clean
