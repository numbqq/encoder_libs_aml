#                   (C) COPYRIGHT 2014 VERISILICON
#                            ALL RIGHTS RESERVED
#
#File: globalrules
#

# Architecture flags for gcc
#ARCH=
#CROSS_COMPILE=
# C -compiler name, can be replaced by another compiler(replace gcc)
#CC = $(CROSS_COMPILE)gcc

# MACRO for cleaning object -files
#RM  = rm -f

# MACRO for creating library that includes all the object files
#AR  = $(CROSS_COMPILE)ar rcs

# Debug compile option dynamically. for example, make hevc DBGOPT="-DTRACE_REGS -DHEVCENC_TRACE"
DEBFLAGS := $(DBGOPT)
# .n.e     current options includes, (e.g.enable by GDBOPT=\"-DTRACE_REGS\")
# .n.e    -DHEVCENC_TRACE/-DTRACE_EWL/-DTRACE_RC/-DTRACE_REGS/-DTRACE_MEM_USAGE"
# instead, using env "VCENC_LOG_TRACE", bitmap bit 0~6: e.g. enable API and EWL, need set env by 'export VCENC_LOG_TRACE=5 #b101'
# 			0: API                               enable api.trc
#            1: REGS:                                  Registers
#            2: EWL  					for wrapper layer trace
#            3: RC                            Rate Control trace
#            4: MEM        for printing allocated EWL mem chunks
#            5: CML
#            6: PERF
# others related env setting:
#   VCNEC_LOG_OUTPUT: output direction (0:stdout | 1: two files TRACE/CHECK | 2: files per thread)
#   VCENC_LOG_LEVEL: output level (0:stdout | 1:FATAL | 2:ERROR | 3:WARN | 4:INFO | 5:DEBUG | 6:ALL)
#   VCENC_LOG_CHECK: enable CHECK, bitmap bit0~4: RECON|QUALITY|VBV|RC|FEATURE

USE_FREERTOS_SIMULATOR ?= n
# In x86_linux, run FreeRTOS, use linux pthread
ifeq ($(strip $(USE_FREERTOS_SIMULATOR)),y)
  DEBUG ?= n # Alarm clock when debug, next to check
  DEBFLAGS += -D__FREERTOS__ -DFREERTOS_SIMULATOR
endif

# Add your debugging flags (or not)
ifeq ($(DEBUG),y)
  DEBFLAGS += -O0 -g -DDEBUG -D_DEBUG_PRINT -D_ASSERT_USED

else
  DEBFLAGS += -O2 -DNDEBUG
endif

# Log message flags (or not)
ifeq ($(LOGMSG),y)
  DEBFLAGS += -DVCE_LOGMSG
endif


ifeq ($(APITRC_STDOUT),y)
  DEBFLAGS += -DAPITRC_STDOUT
endif

ifeq (freertos, $(findstring freertos, $(MAKECMDGOALS)))
  DEBFLAGS += -Wno-unused -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-strict-overflow -Wno-array-bounds -Wno-shift-negative-value # for test data generation
else
  DEBFLAGS += -Werror -Wno-unused -Wno-unused-parameter -Wno-unused-variable -Wno-unused-function -Wno-strict-overflow -Wno-array-bounds -Wno-shift-negative-value # for test data generation
endif
#DEBFLAGS+=-DNO_OUTPUT_WRITE

# This is used for testing with system model
# encoder uses comment header spefified in test bench
# if you want to use own comment header data, comment this out!
DEBFLAGS +=-DTB_DEFINED_COMMENT

ifeq ($(TRACE),y)
  TRACEFLAGS = -DTEST_DATA
else
  TRACEFLAGS =
endif


ifeq ($(shell uname -m),x86_64)
      ifneq (,$(filter $(MAKECMDGOALS),pclinux system testdata eval))
        ifeq ($(USE_32BIT), y)
          export ARCH = -m32
        endif
      endif
endif

# compiler switches
CFLAGS  = $(ARCH) -Wall -D_GNU_SOURCE -D_REENTRANT -D_THREAD_SAFE \
          $(DEBFLAGS) $(INCLUDE) $(INCFLAGS) -fPIC
CFLAGS += -Wmissing-field-initializers -std=gnu99

CFLAGS += -static
CFLAGS += -DCONFIG_COMPAT
GCC_GTEQ_470 := $(shell expr `gcc --version | grep ^gcc | sed 's/^.* //g' | sed -e 's/\.\([0-9][0-9]\)/\1/g' -e 's/\.\([0-9]\)/0\1/g' -e 's/^[0-9]\{3,4\}$$/&00/'` \>= 40700)
ifeq ($(GCC_GTEQ_470),1)
  ifeq ($(ASM),y)
    CFLAGS += -DASM -msse -msse2 -mavx2
  endif
endif

ifeq ($(HEVC),y)
  CFLAGS += -DSUPPORT_HEVC
endif

ifeq ($(H264),y)
  CFLAGS += -DSUPPORT_H264
endif

ifeq ($(AV1),y)
  CFLAGS += -DSUPPORT_AV1
endif

ifeq ($(VP9),y)
  CFLAGS += -DSUPPORT_VP9
endif

ifeq ($(INCLUDE_JPEG),y)
  CFLAGS += -DSUPPORT_JPEG
endif

ifeq ($(PROFILE),y)
CFLAGS += -pg
endif
#CFLAGS += -Wsign-compare

GCC_GTEQ_435 := $(shell expr `gcc --version | grep ^gcc | sed 's/^.* //g' | sed -e 's/\.\([0-9][0-9]\)/\1/g' -e 's/\.\([0-9]\)/0\1/g' -e 's/^[0-9]\{3,4\}$$/&00/'` \>= 40305)
ifeq ($(GCC_GTEQ_435),1)
    CFLAGS += -Wempty-body -Wtype-limits
endif

GCC_GTEQ_440 := $(shell expr `gcc --version | grep ^gcc | sed 's/^.* //g' | sed -e 's/\.\([0-9][0-9]\)/\1/g' -e 's/\.\([0-9]\)/0\1/g' -e 's/^[0-9]\{3,4\}$$/&00/'` \>= 40400)
ifeq ($(GCC_GTEQ_440),1)
    CFLAGS += -Wno-unused-result
endif

# Encoder compiler flags

CFLAGS += -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE -DCTBRC_STRENGTH -fgnu89-inline

ifeq ($(PCIE_FPGA_VERI_LINEBUF),y)
CFLAGS += -DPCIE_FPGA_VERI_LINEBUF
endif

ifeq ($(DAT_COVERAGE), y)
 CFLAGS += -DDAT_COVERAGE
endif

ifeq ($(INTERNAL_TEST),y)
  CFLAGS += -DINTERNAL_TEST
  CONFFLAGS += -DINTERNAL_TEST
endif

ifeq ($(USE_COVERAGE), y)
  CFLAGS += -coverage -fprofile-arcs -ftest-coverage
endif

ifeq ($(VSBTEST), y)
  CFLAGS += -DVSB_TEMP_TEST
endif

ifeq ($(SUPPORT_CACHE),y)
  CFLAGS += -DSUPPORT_CACHE -I$(CMBASE)/cache_software/inc
  CACHELIB = $(CMBASE)/cache_software/linux_reference/libcache.a
endif
ifeq ($(SUPPORT_INPUT_CACHE),y)
  CFLAGS += -DSUPPORT_INPUT_CACHE
endif
ifeq ($(SUPPORT_REF_CACHE),y)
  CFLAGS += -DSUPPORT_REF_CACHE
endif

ifeq ($(strip $(SUPPORT_DEC400)),y)
  CFLAGS += -DSUPPORT_DEC400
endif

ifeq ($(strip $(SUPPORT_AXIFE)),y)
  CFLAGS += -DSUPPORT_AXIFE
endif

ifeq ($(strip $(CMODEL_AXIFE)),y)
  CFLAGS += -DCMODEL_AXIFE
endif

ifeq ($(strip $(SUPPORT_APBFT)),y)
  CFLAGS += -DSUPPORT_APBFT
endif

ifeq ($(strip $(SUPPORT_MEM_SYNC)),y)
  CFLAGS += -DSUPPORT_MEM_SYNC
endif

ifeq ($(strip $(SUPPORT_MEM_STATISTIC)),y)
  CFLAGS += -DSUPPORT_MEM_STATISTIC
endif

ifeq ($(strip $(VIRTUAL_PLATFORM_TEST)),y)
  CFLAGS += -DVIRTUAL_PLATFORM_TEST
endif

ifeq ($(strip $(USE_SAFESTRING)),y)
  ifeq ($(CROSS_COMPILE),)
    SAFESTRINGLIB = $(CMBASE)/software/lib/libsafestring.a
  else
    SAFESTRINGLIB = $(CMBASE)/software/lib/libsafestring_aarch64.a
  endif
endif

ifeq ($(RECON_REF_1KB_BURST_RW),y)
  CFLAGS += -DRECON_REF_1KB_BURST_RW
endif

ifeq ($(RECON_REF_ALIGN64),y)
  CFLAGS += -DRECON_REF_ALIGN64
endif

ifeq ($(SYSTEM), system60)
  CFLAGS += -DSYSTEM60_BUILD
endif

ifeq ($(strip $(SUPPORT_AXIFE)),y)
  CFLAGS += -DSUPPORT_AXIFE
endif

ifeq ($(strip $(CHECKSUM_CRC_BUILD_SUPPORT)),y)
  CFLAGS += -DCHECKSUM_CRC_BUILD_SUPPORT
endif

ifeq ($(strip $(CUTREE_BUILD_SUPPORT)),y)
  CFLAGS += -DCUTREE_BUILD_SUPPORT
endif

ifeq ($(strip $(VCMD_BUILD_SUPPORT)),y)
  CFLAGS += -DVCMD_BUILD_SUPPORT
endif

ifeq ($(strip $(LOW_LATENCY_BUILD_SUPPORT)),y)
  CFLAGS += -DLOW_LATENCY_BUILD_SUPPORT
endif

ifeq ($(strip $(ERROR_BUILD_SUPPORT)),y)
  CFLAGS += -DERROR_BUILD_SUPPORT
endif

ifeq ($(strip $(LINUX_LOCK_BUILD_SUPPORT)),y)
  CFLAGS += -DLINUX_LOCK_BUILD_SUPPORT
endif

ifeq ($(strip $(FIFO_BUILD_SUPPORT)),y)
  CFLAGS += -DFIFO_BUILD_SUPPORT
endif

ifeq ($(strip $(SEI_BUILD_SUPPORT)),y)
  CFLAGS += -DSEI_BUILD_SUPPORT
endif

ifeq ($(strip $(RATE_CONTROL_BUILD_SUPPORT)),y)
  CFLAGS += -DRATE_CONTROL_BUILD_SUPPORT
endif

ifeq ($(strip $(CUINFO_BUILD_SUPPORT)),y)
  CFLAGS += -DCUINFO_BUILD_SUPPORT
endif

ifeq ($(strip $(ROI_BUILD_SUPPORT)),y)
  CFLAGS += -DROI_BUILD_SUPPORT
endif

ifeq ($(strip $(STREAM_CTRL_BUILD_SUPPORT)),y)
  CFLAGS += -DSTREAM_CTRL_BUILD_SUPPORT
endif

# trace recon file by c-model to handle compression and bit-depth
system: TRACEFLAGS += -DTRACE_RECON_BYSYSTEM -DSYSTEM_BUILD
system: CFLAGS += $(TRACEFLAGS)
# for libva
system_multifile: CFLAGS += -DMULTIFILEINPUT

testdata: CFLAGS += $(TRACEFLAGS)

integrator: ENVSET  = -DSDRAM_LM_BASE=0x80000000 \
                      -DENC_MODULE_PATH=\"/dev/vc8000\" \
                      -DMEMALLOC_MODULE_PATH=\"/dev/memalloc\"
integrator: ENVSET  += -DEWL_NO_HW_TIMEOUT -DARM_ARCH_SWAP
integrator: CROSS_COMPILE = arm-linux-
integrator: ARCH = -mcpu=arm9tdmi -mtune=arm9tdmi
integrator: LIB += -lpthread

versatile: ENVSET  = -DSDRAM_LM_BASE=0x00000000 \
                     -DENC_MODULE_PATH=\"/tmp/dev/vc8000\" \
                     -DMEMALLOC_MODULE_PATH=\"/tmp/dev/memalloc\"
versatile: ENVSET  += -DEWL_NO_HW_TIMEOUT  -DARM_ARCH_SWAP
versatile: CROSS_COMPILE = arm-none-linux-gnueabi-
versatile: ARCH = -mcpu=arm926ej-s -mtune=arm926ej-s
versatile: CFLAGS  += $(TRACEFLAGS)
versatile: LIB += -lpthread

versatile_multifile: CROSS_COMPILE = arm-none-linux-gnueabi-
versatile_multifile: ARCH = -mcpu=arm926ej-s -mtune=arm926ej-s
versatile_multifile: LIB += -lpthread
versatile_multifile: CFLAGS += -DMULTIFILEINPUT

CFLAGS  += -DSDRAM_LM_BASE=0x00000000 \
                     -DENC_MODULE_PATH=\"/dev/vc8000\" \
                     -DMEMALLOC_MODULE_PATH=\"/dev/memalloc\"
CFLAGS  += -DEWL_NO_HW_TIMEOUT  -DARM_ARCH_SWAP -DANDROID_EWL
android: ANDROID_PATH=/mnt/file/xiaoqian.zhang/Android/R_aosp
android: CROSS_COMPILE = $(ANDROID_PATH)/builts/gcc/linux-x86/arm/aarch64-linux-android-4.9/bin/
android: ARCH = -fno-exceptions -Wno-multichar -msoft-float -fpic -ffunction-sections -fdata-sections -funwind-tables -fstack-protector -Wa,--noexecstack -Werror=format-security -fno-short-enums -march=armv7-a -mfloat-abi=softfp -mfpu=neon
#Please correct android path
android: CFLAGS  += $(TRACEFLAGS)
android: INCLUDE += \
           -I$(ANDROID_PATH)/bionic/libc/arch-arm/include \
           -I$(ANDROID_PATH)/bionic/libc/include \
           -I$(ANDROID_PATH)/bionic/libstdc++/include \
           -I$(ANDROID_PATH)/bionic/libc/kernel/common \
           -I$(ANDROID_PATH)/bionic/libc/kernel/arch-arm \
           -I$(ANDROID_PATH)/bionic/libm/include \
           -I$(ANDROID_PATH)/bionic/libm/include/arm \
           -I$(ANDROID_PATH)/bionic/libthread_db/include
android: ANDROID_LIB_PATH=$(ANDROID_PATH)/out/target/product/your_product_name/obj/lib/
android: CFLAGS+=  \
           -nostdlib -L$(ANDROID_LIB_PATH)
android: LIB += -lc -lm
android: OBJS += $(ANDROID_LIB_PATH)/crtbegin_dynamic.o $(ANDROID_LIB_PATH)/crtend_android.o

pcdemo: CFLAGS  = -O2 -g -Wall -D_GNU_SOURCE -D_REENTRANT -D_THREAD_SAFE \
                  -DDEBUG -D_ASSERT_USED $(INCLUDE)

pci: CROSS_COMPILE=
pci: ARCH=
pci: ENVSET  = -DPC_PCI_FPGA_DEMO \
               -DSDRAM_LM_BASE=$(CHW_BASE_ADDRESS) \
               -DENC_MODULE_PATH=\"/tmp/dev/vc8000\" \
               -DMEMALLOC_MODULE_PATH=\"/tmp/dev/memalloc\"
pci: ENVSET  += -DEWL_NO_HW_TIMEOUT -DARM_ARCH_SWAP
pci: CFLAGS  += $(TRACEFLAGS)

freertos_lib: CFLAGS += -D__FREERTOS__

lint: LINT_DEF=-dVIDEOSTAB_ENABLED

# object files will be generated from .c sourcefiles
OBJS = $(SRCS:.c=.o)
