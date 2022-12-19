#-------------------------------------------------------------------------------
#-                                                                            --
#-       This software is confidential and proprietary and may be used        --
#-        only as expressly authorized by a licensing agreement from          --
#-                                                                            --
#-                            Verisilicon.					                          --
#-                                                                            --
#-                   (C) COPYRIGHT 2014 VERISILICON.                          --
#-                            ALL RIGHTS RESERVED                             --
#-                                                                            --
#-                 The entire notice above must be reproduced                 --
#-                  on all copies and should not be removed.                  --
#-                                                                            --
#-------------------------------------------------------------------------------
#-
#--  Abstract : Makefile for encoder testbench
#--
#-------------------------------------------------------------------------------

include ../../../build/globaldefs

HEVC ?= y
H264 ?= n
AV1 ?= n
VP9 ?= n
RCSRC ?= n
include ../../../build/globalrules

cmbase = ../../../..
CMBASE = $(shell cd $(cmbase); pwd )

# the path where to find header files
INCFLAGS = -I../../../inc -I../../../source/common -I../../../source/hevc
ifeq ($(TRACE),y)
  INCFLAGS += -I../../debug_trace \
      -I../../../source/camstab
endif

ifeq ($(RCSRC),y)
INCFLAGS += -I../../../../system/models/video/common/
endif

ifeq (freertos, $(findstring freertos, $(MAKECMDGOALS)))
  FREERTOS_POSIX_DIR = ../../ewl/osal/freertos/lib
  FREERTOS_KERNEL_DIR = ../../ewl/osal/freertos/FreeRTOS_Kernel/Source
	INCFLAGS += -I$(FREERTOS_POSIX_DIR)/FreeRTOS-Plus-POSIX/include/portable/pc/linux \
                -I$(FREERTOS_POSIX_DIR)/FreeRTOS-Plus-POSIX/include/portable \
                -I$(FREERTOS_POSIX_DIR)/FreeRTOS-Plus-POSIX/include \
                -I$(FREERTOS_POSIX_DIR)/include/private \
                -I$(FREERTOS_POSIX_DIR)/include \
                -I$(FREERTOS_KERNEL_DIR)/include \
                -I$(FREERTOS_KERNEL_DIR)/portable/GCC/Posix \
                -I../../kernel_module/freertos
    CFLAGS += -D__FREERTOS__
endif

ifeq ($(AV1),y)
	INCFLAGS += -I../../../source/av1
endif

ifeq ($(VP9),y)
	INCFLAGS +=	-I../../../source/vp9
endif

ifeq ($(strip $(USE_FREERTOS_SIMULATOR)),y)
  FreeRTOSDir = ../../ewl/osal/freertos/FreeRTOS_Kernel
  INCFLAGS += -I$(FreeRTOSDir)/Source/include \
              -I$(FreeRTOSDir)/Source/portable/GCC/Posix
  include $(FreeRTOSDir)/Source/Makefile
endif

ifneq ($(target),)
  ifneq ($(target), default)
    CFLAGS += -DTARGET -D$(target)
  endif
endif

ifneq ($(P4CL),)
	CFLAGS += -DP4CL=$(P4CL)
endif

# list of used sourcefiles
SRCS = test_bench.c test_bench_utils.c get_option.c test_bench_pic_config.c tb_flex_ref.c tb_input_file_list.c tb_parse_roi.c

ifeq ($(INTERNAL_TEST),y)
# for internal tests
SRCS += test_bench_statistic.c
endif

vpath %.c
vpath %.c ../../../source/hevc

LIB = ../../libh2enc.a $(SAFESTRINGLIB)
ifeq ($(strip $(USE_FREERTOS_SIMULATOR)),y)
  FREERTOS_KERNEL_LIB = ../../libfreertos.a
endif
FREERTOS_LIB = ../../kernel_module/freertos/libenc_freertos_driver_static.a ../../libfreertos.a
FREERTOS_LIB_PATH = -L../../ -L../../kernel_module/freertos

# System model library
MODELLIB = $(CMBASE)/$(SYSTEM)/models/ench2_asic_model.a
ifeq ($(VMAF_SUPPORT),y)
VMAFLIB = -L$(CMBASE)/software/lib/ -lvmaf -lstdc++
CFLAGS += -DVMAF_SUPPORT
endif

# name of the output executable
TARGET = hevc_testenc


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
$(MODELLIB):
	$(MAKE) -w -C ../../../../$(SYSTEM)/models HEVC=$(HEVC) H264=$(H264) AV1=$(AV1) VP9=$(VP9)

$(LIB):
	$(MAKE) -w -C ../.. $(TARGETENV) HEVC=$(HEVC) H264=$(H264) AV1=$(AV1) VP9=$(VP9) \
                PCIE_FPGA_VERI_LINEBUF=$(PCIE_FPGA_VERI_LINEBUF)

version.h:../../../../.git/HEAD ../../../../.git/index
	echo "const char *git_version = \"$(shell git rev-parse HEAD)\";" > $@



.PHONY: eval
evaluation: eval
eval: TARGET = hevc_enc_h2_eval
eval: TARGETENV = eval
eval: $(LIB) $(OBJS) $(MODELLIB)
	$(CC) $(CFLAGS) $(OBJS) $(LIB) $(MODELLIB) $(VMAFLIB) -lm -lpthread -o $(TARGET)

pclinux: system

.PHONY: system
system: TARGETENV = system
system: $(VERSION_H) .depend $(LIB) $(OBJS) $(CACHELIB) $(MODELLIB)
	$(CC) $(CFLAGS) $(OBJS) $(LIB) $(MODELLIB) $(VMAFLIB) $(CACHELIB) ${FREERTOS_KERNEL_LIB} -lm -lpthread -o $(TARGET)

.PHONY: system_multifile
system_multifile: TARGET = hevc_testenc_multifile
system_multifile: TARGETENV = system
system_multifile: $(VERSION_H) .depend $(LIB) $(OBJS) $(MODELLIB)
	$(CC) $(CFLAGS) $(OBJS) $(LIB) $(MODELLIB) $(VMAFLIB) -lm -lpthread -o $(TARGET)

.PHONY: system_static
system_static: TARGETENV = system
system_static: $(VERSION_H) .depend $(LIB) $(OBJS) $(MODELLIB)
	$(CC) $(CFLAGS) $(OBJS) $(LIB) $(MODELLIB) $(VMAFLIB) -lm -lpthread -o $(TARGET) -static

.PHONY: versatile
versatile: TARGETENV = versatile
versatile: $(VERSION_H) .depend $(OBJS)
	$(MAKE) -w -C ../.. $@ \
            CROSS_COMPILE=$(CROSS_COMPILE) ARCH="$(ARCH)"
	$(CC) $(CFLAGS) $(OBJS) $(LIB) $(CACHELIB) -lm -lpthread -o $(TARGET)

.PHONY: versatile_multifile
versatile_multifile: TARGETENV = versatile
versatile_multifile: TARGET = hevc_testenc_multifile
versatile_multifile: .depend $(OBJS)
	$(MAKE) -w -C ../.. versatile \
            CROSS_COMPILE=$(CROSS_COMPILE) ARCH="$(ARCH)"
	$(CC) $(CFLAGS) $(OBJS) $(LIB) -o $(TARGET)

.PHONY: integrator
integrator: TARGETENV = integrator
integrator: $(OBJS)
	$(MAKE) -w -C ../.. $@ \
            CROSS_COMPILE=$(CROSS_COMPILE) ARCH="$(ARCH)"
	$(CC) $(CFLAGS) $(OBJS) $(LIB) -o $(TARGET)

.PHONY: pci
pci: TARGETENV = pci
pci: $(VERSION_H) .depend $(LIB) $(OBJS) $(CACHELIB)
	$(CC) $(CFLAGS) $(OBJS) $(LIB) $(CACHELIB) $(VMAFLIB) -lm -lpthread -o $(TARGET)

.PHONY: freertos
freertos:TARGETENV = freertos
freertos:$(VERSION_H) .depend $(LIB) $(OBJS) $(CACHELIB)
	$(CC) $(CFLAGS) $(OBJS) $(LIB) $(CACHELIB) $(VMAFLIB) -lm $(FREERTOS_LIB) -o $(TARGET)


system_cov: CC = covc --retain -t!HEVCTestBench.c,!EncGetOption.c g++
system_cov: TARGETENV = system_cov
system_cov: $(MODELLIB) $(LIB) $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LIB) $(MODELLIB) $(VMAFLIB) -o $(TARGET)

.PHONY: android
android: TARGETENV = android
android: $(VERSION_H) $(OBJS)
	$(MAKE) -w -C ../.. $@ \
            CROSS_COMPILE=$(CROSS_COMPILE) ARCH="$(ARCH)"
	$(CC) $(CFLAGS) $(OBJS)  $(LIB)  -o $(TARGET)

.PHONY: clean
clean:
	$(RM) .depend
	$(RM) *.o *.gcno *.gcda core* *~ $(TARGET) $(TARGET).* $(VERSION_H)

.PHONY: libclean
libclean: clean
	$(MAKE) -w -C ../.. clean

.PHONY: tags
tags:
	ctags ../../../inc/*.h *.c ../../../source/hevc/*.[ch] \
            ../../../source/common/*.[ch] ../../ewl/*c

depend .depend: $(SRCS)
	@echo -e "\n [DEP]\t $^"
	@awk '{gsub(/```/,"");print}' ../vcenc_help.md | xxd -i > ../vcenc_help.dat
	@$(CC) -M $(DEBFLAGS) $(INCFLAGS) $^ > .depend

ifeq (,$(findstring clean, $(MAKECMDGOALS)))
ifeq (.depend, $(wildcard .depend))
include .depend
endif
endif

