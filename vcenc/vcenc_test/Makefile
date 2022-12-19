include ../build/globaldefs

HEVC ?= y

include ../build/globalrules


# the path where to find header files
INCFLAGS = -I../inc -I../source/common -I../source/hevc

ifneq ($(target),)
  ifneq ($(target), default)
    CFLAGS += -DTARGET -D$(target)
  endif
endif

# list of used sourcefiles
SRCS = test_bench.c test_bench_utils.c get_option.c test.c

vpath %.c
vpath %.c ../source/hevc

LIB = ../linux_reference/libh2enc.a

# name of the output executable
TARGET = vc_test_enc


#Here are rules for building codes and generating executable
all:    $(VERSION_H) tags
	@echo ---------------------------------------
	@echo "Usage: make [ system | eval | versatile | android ]"
	@echo "system     - PC system model (== pclinux)"
	@echo "eval       - PC system model for evaluation with frame limit"
	@echo "pci        - PC with FPGA HW"
	@echo "integrator - ARM integrator with FPGA HW"
	@echo "versatile  - ARM versatile with FPGA HW"
	@echo "android    - ARM android"
	@echo "freertos   - No sepcial cpu, just build with gcc in Linux"
	@echo "NOTE! Make sure to do 'make clean libclean'"
	@echo "between compiling to different targets!"
	@echo ---------------------------------------

# Use other makefiles to build the libraries

$(LIB):
	$(MAKE) -w -C ../linux_reference $(TARGETENV) HEVC=$(HEVC)

version.h:../../.git/HEAD ../../.git/index
	echo "const char *git_version = \"$(shell git rev-parse HEAD)\";" > $@

.PHONY: pci
pci: TARGETENV = pci
pci: $(VERSION_H) .depend $(LIB) $(OBJS) $(CACHELIB)
	$(CC) $(CFLAGS) $(OBJS) $(LIB) $(CACHELIB) $(VMAFLIB) -lm -lpthread -static -o $(TARGET)

.PHONY: clean
clean:
	$(RM) .depend
	$(RM) *.o *.gcno *.gcda core* *~ $(TARGET) $(TARGET).* $(VERSION_H)

.PHONY: libclean
libclean: clean
	$(MAKE) -w -C ../linux_reference clean

.PHONY: tags
tags:
	ctags ../inc/*.h *.c ../source/hevc/*.[ch] \
            ../source/common/*.[ch] ../linux_reference/ewl/*c

depend .depend: $(SRCS)
	@echo -e "\n [DEP]\t $^"
	@awk '{gsub(/```/,"");print}' ./vcenc_help.md | xxd -i > ./vcenc_help.dat
	@$(CC) -M $(DEBFLAGS) $(INCFLAGS) $^ > .depend

ifeq (,$(findstring clean, $(MAKECMDGOALS)))
ifeq (.depend, $(wildcard .depend))
include .depend
endif
endif

