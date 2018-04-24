# 指令编译器和选项
ifeq ($(ARM), 1)
CC=arm-none-linux-gnueabi-g++
AR=arm-none-linux-gnueabi-ar
else
CC=gcc
AR=ar
endif

ifeq ($(ARM), 1)
CFLAGS+=-DARM
else
CFLAGS+=-O2 -std=c99
endif
CFLAGS+= -g -static
# 目标文件
TARGET = ../libEnc.a
LDFLAGS += -lm -lrt

INC = -I./include -I./enc/gx_enc_fast -I.

../libEnc.a:libvpcodec.o AML_HWEncoder.o enc_api.o gxvenclib_fast.o rate_control_gx_fast.o parser.o
	$(AR) rcs $@ $^

libvpcodec.o: libvpcodec.cpp
	$(CC) -c $(CFLAGS) $(INC) $< -o $@

AML_HWEncoder.o: AML_HWEncoder.cpp
	$(CC) -c $(CFLAGS) $(INC) $< -o $@

enc_api.o: enc_api.cpp
	$(CC) -c $(CFLAGS) $(INC) $< -o $@

gxvenclib_fast.o: ./enc/gx_enc_fast/gxvenclib_fast.cpp
	$(CC) -c $(CFLAGS) $(INC) $< -o $@

rate_control_gx_fast.o: ./enc/gx_enc_fast/rate_control_gx_fast.cpp
	$(CC) -c $(CFLAGS) $(INC) $< -o $@

parser.o: ./enc/gx_enc_fast/parser.cpp
	$(CC) -c $(CFLAGS) $(INC) $< -o $@

clean:
	-rm -f *.o
	-rm -f $(TARGET)