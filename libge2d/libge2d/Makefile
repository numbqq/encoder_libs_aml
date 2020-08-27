export CROSS_COMPILE=aarch64-linux-gnu-
CC = $(CROSS_COMPILE)gcc

CFLAGS += -I./../../libion/include/ion

SRC  := $(wildcard *.c)
OBJS := $(SRC:%.c=%.o)
.PHONY : clean all

LIB := libge2d.so

LIBS += -L../../libion

all: $(LIB)

%.o: %.c
	$(CC) -c -fPIC -o $@ $< $(CFLAGS)

$(LIB): $(OBJS)
	$(CC) -shared -fPIC -lion -o $(LIB) $(OBJS) $(LIBS)
	@rm -f $(OBJS)

clean:
	rm -f $(LIB)
