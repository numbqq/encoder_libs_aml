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
#--  Abstract : Makefile for encoder library
#--
#-------------------------------------------------------------------------------



HEVC ?= y
H264 ?= y
AV1 ?= n
VP9 ?= n
RCSRC ?= n

include ../build/globaldefs
include ../build/globalrules

cmbase = ../..
CMBASE = $(shell cd $(cmbase); pwd )

ifeq (pci,$(findstring pci, $(MAKECMDGOALS)))
# this is just for DEMO
include Baseaddress
endif

# System model library
MODELLIB = ../../$(SYSTEM)/models/ench2_asic_model.a

# show the path to compiler, where to find header files
INCLUDE = -I../source/hevc -I../source/jpeg -I../source/camstab -I../inc -I../source/common \
          -Idebug_trace -I../source/av1 -I../source/vp9
ifeq ($(RCSRC),y)
INCLUDE += -I../../system/models/video/common/
endif

# avoid reporting -Werror=misleading-indentation when code-obfuscator
ifeq ($(RCSRC),n)
CFLAGS += -Wno-misleading-indentation
endif

ifeq ($(strip $(USE_SAFESTRING)),y)
  CFLAGS += -DSAFESTRING
  INCLUDE += -I../inc/safestring
endif

ifeq (pci,$(findstring pci, $(MAKECMDGOALS)))
  INCLUDE += -Ikernel_module/linux -Imemalloc/pcie
else
ifeq (freertos, $(findstring freertos, $(MAKECMDGOALS)))
  INCLUDE += -Ikernel_module/freertos -Ikernel_module/linux
else
ifeq (freertos_lib, $(findstring freertos_lib, $(MAKECMDGOALS)))
  INCLUDE += -Ikernel_module/freertos -Ikernel_module/linux
else
  #INCLUDE += -Ikernel_module -Imemalloc
  INCLUDE += -Ikernel_module/gpl -Imemalloc/gpl -Ikernel_module/linux
endif
endif
endif

ifeq (freertos, $(findstring freertos, $(MAKECMDGOALS)))
  include ewl/osal/freertos/freertos.mk
  CFLAGS += -DEWLHWCFGFILE=\"hwcfg/ewl_hwcfg$(FEATURES).c\"
  override CFLAGS:=$(filter-out -pedantic,$(CFLAGS))
  override CFLAGS:=$(filter-out -Werror,$(CFLAGS))
endif
ifeq (freertos_lib, $(findstring freertos_lib, $(MAKECMDGOALS)))
  include ewl/osal/freertos/freertos.mk
  CFLAGS += -DEWLHWCFGFILE=\"hwcfg/ewl_hwcfg$(FEATURES).c\"
  override CFLAGS:=$(filter-out -pedantic,$(CFLAGS))
  override CFLAGS:=$(filter-out -Werror,$(CFLAGS))
endif
FREERTOS_KERNEL_DIR = ewl/osal/freertos/FreeRTOS_Kernel/Source
FREERTOS_POSIX_SOURCE_DIR = ewl/osal/freertos/lib/FreeRTOS-Plus-POSIX/source

ifeq ($(strip $(USE_FREERTOS_SIMULATOR)),y)
  FreeRTOSDir = ewl/osal/freertos/FreeRTOS_Kernel
  INCLUDE += -I$(FreeRTOSDir)/Source/include \
             -I$(FreeRTOSDir)/Source/portable/GCC/Posix
  include $(FreeRTOSDir)/Source/Makefile
endif

# list of used sourcefiles
SRC_ENC_COMMON := encasiccontroller_v2.c \
                  encasiccontroller.c \
                  queue.c \
                  hash.c \
                  sw_put_bits.c \
                  tools.c \
                  encpreprocess.c \
                  apbfilter.c \
		          pool.c

ifeq ($(SUPPORT_AXIFE),y)
SRC_ENC_COMMON += axife.c
endif

ifeq ($(CHECKSUM_CRC_BUILD_SUPPORT),y)
SRC_ENC_COMMON += checksum.c
SRC_ENC_COMMON += crc.c
endif

ifeq ($(LOGMSG),y)
SRC_ENC_COMMON += enc_log.c
endif

ifeq ($(SUPPORT_DEC400),y)
SRC_ENC_COMMON += encdec400.c
endif

ifeq ($(LOW_LATENCY_BUILD_SUPPORT),y)
SRC_ENC_COMMON += encinputlinebuffer.c
endif

ifeq ($(ERROR_BUILD_SUPPORT),y)
SRC_ENC_COMMON += error.c
endif

ifeq ($(FIFO_BUILD_SUPPORT),y)
SRC_ENC_COMMON += fifo.c
endif

ifeq ($(RATE_CONTROL_BUILD_SUPPORT),y)
ifeq ($(RCSRC),y)
SRC_ENC_COMMON += ../../system/models/video/common/rate_control_picture.c
RCPATH = ../../system/models/video/common/rate_control_picture.o
else
SRC_ENC_COMMON += rate_control_picture.c
RCPATH = ../source/hevc/rate_control_picture.o
endif
endif


SRC_HEVC := hevcencapi.c\
            hevcencapi_utils.c\
            sw_picture.c \
            sw_parameter_set.c \
            sw_slice.c\
            sw_nal_unit.c

ifeq ($(CUTREE_BUILD_SUPPORT),y)
SRC_ENC_COMMON += cutree_stream_ctrl.c
SRC_ENC_COMMON += cutreeasiccontroller.c
SRC_ENC_COMMON += sw_cu_tree.c
endif

ifeq ($(SUPPORT_CACHE),y)
SRC_ENC_COMMON += hevcenccache.c
endif

ifeq ($(SEI_BUILD_SUPPORT),y)
SRC_ENC_COMMON += hevcSei.c
endif

ifeq ($(CUINFO_BUILD_SUPPORT),y)
SRC_ENC_COMMON += vcenc_cuinfo.c
endif

ifeq ($(ROI_BUILD_SUPPORT),y)
SRC_ENC_COMMON += vcenc_fillroimap.c
endif

ifeq ($(STREAM_CTRL_BUILD_SUPPORT),y)
SRC_ENC_COMMON += vcenc_stream_ctrl.c
endif



ifeq ($(VMAF_SUPPORT),y)
SRC_HEVC += vmaf.c
VMAF_MODEL = vmaf_v0.6.1
CFLAGS += -DVMAF_MODEL=$(VMAF_MODEL)
../source/common/vmaf.pkl.h: ../lib/$(VMAF_MODEL).pkl
	xxd -i $< $@
../source/common/vmaf.pkl.model.h: ../lib/$(VMAF_MODEL).pkl.model
	xxd -i $< $@
# generate headers before dependency check
depend .depend: ../source/common/vmaf.pkl.h ../source/common/vmaf.pkl.model.h
endif

SRC_VP9  := 	vp9encapi.c \
		vp9_bitstream.c \
		vp9_subexponential.c \
		vp9_entropymode.c \
		vp9_entropy_mv.c

SRC_AV1 := av1encapi.c \
           av1_prob_init.c \
           av1enc_metadata.c


SRC_JPEG := EncJpeg.c\
            EncJpegInit.c\
            EncJpegCodeFrame.c\
            EncJpegPutBits.c\
            JpegEncApi.c \
            MjpegEncApi.c

ifeq ($(RATE_CONTROL_BUILD_SUPPORT),y)
SRC_JPEG += rate_control_jpeg.c
endif

SRC_VS   := vidstabalg.c\
            vidstabapi.c\
            vidstabcommon.c\
            vidstabinternal.c


SRC_TRACE = enctrace.c

#test_data.c

# Source files for test case specific test IDs, compiler flag INTERNAL_TEST
# If these are not included some tests will fail


ifeq ($(INTERNAL_TEST),y)
SRC_TESTING = sw_test_id.c
endif

SRC_EWL_PC := ewl_x280_file.c
SRC_EWL_ARM = ewl.c
SRC_EWL_FreeRTOS = ewl.c


ifeq ($(VCMD_BUILD_SUPPORT),y)
    SRC_EWL_ARM += cwl_vc8000_vcmd_common.c
    SRC_EWL_FreeRTOS += cwl_vc8000_vcmd_common.c
endif

ifeq ($(LINUX_LOCK_BUILD_SUPPORT),y)
    SRC_EWL_ARM += ewl_linux_lock.c
endif

ifeq ($(SUPPORT_MEM_SYNC),y)
    SRC_EWL_ARM += ewl_memsync.c
    SRC_EWL_FreeRTOS += ewl_memsync.c
endif

ifeq ($(POLLING_ISR),y)
    CFLAGS += -DPOLLING_ISR
    CFLAGS += -DENCH2_IRQ_DISABLE=1
endif
SRC_EWL_SYS := ewl_vcmd_system.c\
               vwl.c
SRC_EWL_SYS += ewl_system.c
SRC_EWL_SYS += ewl_cmodel.c
SRC_EWL_SYS += ewl_memsync_sys.c

ifeq ($(LIBVA), y)
    SRC_EWL_SYS = vwl.c
    SRC_EWL_ARM =
endif

SRCS = encswhwregisters.c ewl_common.c buffer_info.c

# common parts only for encoder and not for video stab
INCLUDE_ENC_COMMON=y


# Combine the lists of all the source files included in the library build
ifeq ($(INCLUDE_HEVC),y)
    SRCS += $(SRC_HEVC)
endif
ifeq ($(INCLUDE_AV1),y)
    SRCS += $(SRC_AV1)
endif
ifeq ($(INCLUDE_VP9),y)
    SRCS += $(SRC_VP9)
endif
ifeq ($(INCLUDE_JPEG),y)
    SRCS += $(SRC_JPEG)
endif
ifeq ($(INCLUDE_VS),y)
    SRCS += $(SRC_VS)
    CFLAGS += -DVIDEOSTAB_ENABLED
endif
ifeq ($(INTERNAL_TEST),y)
    SRCS += $(SRC_TESTING)
endif

# add common encoder files
ifeq ($(INCLUDE_ENC_COMMON),y)
    SRCS += $(SRC_ENC_COMMON)
endif

# if tracing flags are defined we need to compile the tracing functions
ifeq ($(TRACE),y)
    SRCS += $(SRC_TRACE)
endif

ifneq ($(target),)
  ifneq ($(target),default)
    CFLAGS += -DTARGET -D$(target)
  endif
endif

CFLAGS += -DEWLHWCFGFILE=\"hwcfg/ewl_hwcfg$(FEATURES).c\"

# choose EWL source, system model uses its own EWL
ifneq (,$(findstring pclinux, $(MAKECMDGOALS)))
    SRCS += $(SRC_EWL_SYS)
    INCLUDE += -I../../system/models/inc -Iewl -Iewl/system
    EWL_DIR = system
else
ifneq (,$(findstring system, $(MAKECMDGOALS)))
    SRCS += $(SRC_EWL_SYS)
    INCLUDE += -I../../system/models/inc -Iewl -Iewl/system
    EWL_DIR = system
else
ifneq (,$(findstring testdata, $(MAKECMDGOALS)))
    SRCS += $(SRC_EWL_SYS)
    INCLUDE += -I../../system/models/inc -Iewl -Iewl/system
    EWL_DIR = system
else
ifneq (,$(findstring eval, $(MAKECMDGOALS)))
    SRCS += $(SRC_EWL_SYS)
    INCLUDE += -I../../system/models/inc -Iewl -Iewl/system
    EWL_DIR = system
else
ifeq (freertos_lib, $(findstring freertos_lib, $(MAKECMDGOALS)))
    SRCS += $(SRC_EWL_FreeRTOS)
    INCLUDE += -Iewl
    EWL_DIR =
else
ifeq (freertos, $(findstring freertos, $(MAKECMDGOALS)))
    SRCS += $(SRC_EWL_FreeRTOS)
    INCLUDE += -Iewl
    EWL_DIR =
else
    SRCS += $(SRC_EWL_ARM)
    INCLUDE += -Iewl
    EWL_DIR =
endif
endif
endif
endif
endif
endif

#source search path
vpath %.c
vpath %.c ../source/common ../source/hevc ../source/vp9 ../source/av1 ../source/jpeg ../source/camstab ./ewl/$(EWL_DIR) ./ewl/ ./debug_trace

# name of the outputfile (library)
ENCFREERTOS_LIB = libfreertos.a
FREERTOS_OBJS = $(FREERTOS_C_SRCS:.c=.o)
ENCLIB = libh2enc.a
ENCSLIB = libh2enc.so

#Here are rules for building codes and generating object library.
all:    tags
	@echo ---------------------------------------
	@echo "Usage: make [ system | testdata | versatile | integrator | android]"
	@echo "system     - PC system model (== pclinux)"
	@echo "testdata   - PC system model for test data creation"
	@echo "eval       - PC system model for evaluation with frame limit"
	@echo "versatile  - ARM versatile with FPGA HW"
	@echo "integrator - ARM integrator with FPGA HW"
	@echo "android    - ARM android"
	@echo "NOTE! Make sure to do 'make clean'"
	@echo "between compiling to different targets!"
	@echo ---------------------------------------

.PHONY: pclinux system_lib system testdata integrator versatile android freertos_lib freertos clean tags depend

evaluation: eval
eval: system

pclinux: testdata
system_static: testdata
system: testdata

# for libva
system_lib: system

ifeq ($(strip $(USE_FREERTOS_SIMULATOR)),y)
testdata: .depend $(ENCLIB) $(ENCFREERTOS_LIB)
else
testdata: .depend $(ENCLIB)
endif

integrator: $(ENCLIB)
versatile: $(ENCLIB)

android: $(ENCLIB)

hwlinux: $(ENCLIB)

versatile_tb: versatile
	$(MAKE) -w -C test/vp9 versatile
	$(MAKE) -w -C test/hevc versatile



pcdemo: $(SRC_HEVC:.c=.o) $(SRC_ENC_COMMON:.c=.o)
	$(AR) $(ENCLIB) $+


.PHONY: pci
pci:
pci: $(ENCLIB) $(ENCSLIB)

freertos: freertos_lib
freertos_lib: $(ENCFREERTOS_LIB) $(ENCLIB)
	cd ./kernel_module/freertos && ./drive_cmake.sh

.PHONY: version
version:
	../../system/tools/p4/p4_gen_ver.sh


$(ENCFREERTOS_LIB): $(FREERTOS_OBJS)
	@echo "[AR] $@"
	@$(AR) $@ $^

system_cov: $(ENCLIB)
system_cov: CC = covc --retain -t!debug_trace/enctrace.c,!debug_trace/enctracestream.c g++

$(ENCLIB): $(OBJS)
	@echo "[AR] $@"
	$(AR) $(ENCLIB) $(OBJS)

ifeq (freertos_lib, $(findstring freertos_lib, $(MAKECMDGOALS)))
$(ENCSLIB): $(OBJS)
	$(CC) -fPIC -Wl,-no-undefined -shared $(CFLAGS) $(OBJS) -lm -L. -lfreertos -o libh2enc.so
else ifeq ($(SUPPORT_CACHE),y)
$(ENCSLIB): $(OBJS)
	$(CC) -fPIC -Wl,-no-undefined -shared $(CFLAGS) $(OBJS) -lm -lpthread -L$(CMBASE)/cache_software/linux_reference/ -lcache -o libh2enc.so
else ifeq ($(strip $(USE_SAFESTRING)),y)
$(ENCSLIB): $(OBJS)
	$(CC) -fPIC -Wl,-no-undefined -shared $(CFLAGS) $(OBJS) -lm -lpthread -L$(CMBASE)/software/lib/ -lsafestring -o libh2enc.so
else
$(ENCSLIB): $(OBJS)
	$(CC) -fPIC -Wl,-no-undefined -shared $(CFLAGS) $(OBJS) -lm -lpthread -o libh2enc.so
endif


%.o: %.c
	$(CC) -c $(CFLAGS) $(ENVSET) $(CONFFLAGS) $< -o $@

clean:
	$(RM) $(ENCLIB) $(ENCSLIB)
	$(RM) .depend
	$(RM) *.o *.gcno *.gcda $(RCPATH)
	$(RM) $(ENCFREERTOS_LIB)
	$(RM) $(FREERTOS_KERNEL_DIR)/*.o
	$(RM) $(FREERTOS_KERNEL_DIR)/portable/MemMang/*.o
	$(RM) $(FREERTOS_KERNEL_DIR)/portable/GCC/Posix/*.o
	$(RM) $(FREERTOS_POSIX_SOURCE_DIR)/*.o

tags:
	ctags ../inc/*h ../source/common/*[ch] ../source/hevc/*[ch] ../source/vp9/*[ch]
	#../source/camstab/*[ch]\


lint: $(SRC_HEVC) $(SRC_VP9) $(SRC_JPEG) $(SRC_VS) # take out any not relevant sources
        # -e537 removes warnings about multiple include
        # -e641 removes warnings about converting enum to int
	-lint-nt -w2 -e537 -e641 $(LINT_DEF) $(INCLUDE) $^ > pc-lint-report.txt

depend .depend: $(SRCS)
	$(CC) $(CFLAGS) $(ENVSET) $(CONFFLAGS) -M  $^ > .depend

ifneq (clean, $(findstring clean, $(MAKECMDGOALS)))
ifeq (.depend, $(wildcard .depend))
include .depend
endif
endif
