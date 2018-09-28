# object building.
ifeq ($(ARM), 1)
CC=arm-none-linux-gnueabi-g++
AR=arm-none-linux-gnueabi-ar
else
CC=gcc
AR=ar
endif

TARGET=testApi
AMLENC_LIB=test.o
LinkIn=libEnc.a
LDFLAGS += -lm -lrt
ifeq ($(ARM), 1)
CFLAGS+=-DARM
else
CFLAGS+=-O2 -std=c99
endif
CFLAGS+= -g -static
$(TARGET):$(AMLENC_LIB)
	$(CC) $(CFLAGS) $(AMLENC_LIB) -o $(TARGET) $(LinkIn)
#	$(AR) rcs $@ $(AMLENC_LIB)
#	mkdir example
#	cp $(TARGET) ./example
$(AMLENC_LIB):%.o:%.cpp
	$(CC) -c $(CFLAGS) $< -o $@
clean:
	-rm -f *.o
	-rm -f $(TARGET)