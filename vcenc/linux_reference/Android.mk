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
LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_MODULE            := libvcenc
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/lib
LOCAL_MODULE_PATH_64 := $(TARGET_OUT_VENDOR)/lib64
endif


HEVC ?= y
H264 ?= y
AV1 ?= n
VP9 ?= n
RCSRC ?= n

VCENC_PATH := $(LOCAL_PATH)/../
include $(VCENC_PATH)/build/globaldefs.mk
include $(VCENC_PATH)/build/globalrules.mk

cmbase = ../..
CMBASE = $(shell cd $(cmbase); pwd )

ifeq (pci,$(findstring pci, $(MAKECMDGOALS)))
# this is just for DEMO
include Baseaddress
endif

# System model library
MODELLIB = ../../$(SYSTEM)/models/ench2_asic_model.a

# show the path to compiler, where to find header files
LOCAL_C_INCLUDES        := \
                           $(VCENC_PATH)/source/hevc \
                           $(VCENC_PATH)/source/common \
                           $(VCENC_PATH)/linux_reference/debug_trace \
                           $(VCENC_PATH)/inc \
                           $(VCENC_PATH)/linux_reference/ewl
ifeq ($(RCSRC),y)
LOCAL_C_INCLUDES += $(VCENC_PATH)/system/models/video/common/
#INCLUDE += -I../../system/models/video/common/
endif

# avoid reporting -Werror=misleading-indentation when code-obfuscator
ifeq ($(RCSRC),n)
LOCAL_CFLAGS += -Wno-misleading-indentation
endif

ifeq ($(strip $(USE_SAFESTRING)),y)
  LOCAL_CFLAGS += -DSAFESTRING
  LOCAL_C_INCLUDES += $(VCENC_PATH)/inc/safestring
  #INCLUDE += -I../inc/safestring
endif

ifeq (pci,$(findstring pci, $(MAKECMDGOALS)))
  LOCAL_C_INCLUDES += $(LOCAL_PATH)/kernel_module/linux -Imemalloc/pcie
  #INCLUDE += -Ikernel_module/linux -Imemalloc/pcie
else
ifeq (freertos, $(findstring freertos, $(MAKECMDGOALS)))
LOCAL_C_INCLUDES += $(LOCAL_PATH)/kernel_module/freertos -Ikernel_module/linux
#  INCLUDE += -Ikernel_module/freertos -Ikernel_module/linux
else
ifeq (freertos_lib, $(findstring freertos_lib, $(MAKECMDGOALS)))
LOCAL_C_INCLUDES += $(LOCAL_PATH)/kernel_module/freertos -Ikernel_module/linux
#  INCLUDE += -Ikernel_module/freertos -Ikernel_module/linux
else
  #INCLUDE += -Ikernel_module -Imemalloc
  LOCAL_C_INCLUDES += $(LOCAL_PATH)/kernel_module/gpl -Imemalloc/gpl -Ikernel_module/linux
#  INCLUDE += -Ikernel_module/gpl -Imemalloc/gpl -Ikernel_module/linux
endif
endif
endif

FREERTOS_KERNEL_DIR = ewl/osal/freertos/FreeRTOS_Kernel/Source
FREERTOS_POSIX_SOURCE_DIR = ewl/osal/freertos/lib/FreeRTOS-Plus-POSIX/source

# list of used sourcefiles
SRC_ENC_COMMON := ../source/common/encasiccontroller_v2.c \
                  ../source/common/encasiccontroller.c \
                  ../source/common/queue.c \
                  ../source/common/hash.c \
                  ../source/common/sw_put_bits.c \
                  ../source/common/tools.c \
                  ../source/common/encpreprocess.c \
                  ../source/common/apbfilter.c \
                  ../source/common/pool.c

ifeq ($(SUPPORT_AXIFE),y)
SRC_ENC_COMMON += ../source/common/axife.c
endif

ifeq ($(CHECKSUM_CRC_BUILD_SUPPORT),y)
SRC_ENC_COMMON += ../source/common/checksum.c
SRC_ENC_COMMON += ../source/common/crc.c
endif

ifeq ($(LOGMSG),y)
SRC_ENC_COMMON += ../source/common/enc_log.c
endif

ifeq ($(SUPPORT_DEC400),y)
SRC_ENC_COMMON += ../source/common/encdec400.c
endif

ifeq ($(LOW_LATENCY_BUILD_SUPPORT),y)
SRC_ENC_COMMON += ../source/common/encinputlinebuffer.c
endif

ifeq ($(ERROR_BUILD_SUPPORT),y)
SRC_ENC_COMMON += ../source/common/error.c
endif

ifeq ($(FIFO_BUILD_SUPPORT),y)
SRC_ENC_COMMON += ../source/common/fifo.c
endif

ifeq ($(RATE_CONTROL_BUILD_SUPPORT),y)
ifeq ($(RCSRC),y)
SRC_ENC_COMMON += ../source/hevc/rate_control_picture.c
RCPATH = ../../system/models/video/common/rate_control_picture.o
else
SRC_ENC_COMMON += ../source/hevc/rate_control_picture.c
RCPATH = ../source/hevc/rate_control_picture.o
endif
endif


SRC_HEVC := ../source/hevc/hevcencapi.c\
            ../source/hevc/hevcencapi_utils.c\
            ../source/hevc/sw_picture.c \
            ../source/hevc/sw_parameter_set.c \
            ../source/hevc/sw_slice.c\
            ../source/hevc/sw_nal_unit.c

ifeq ($(CUTREE_BUILD_SUPPORT),y)
SRC_ENC_COMMON += ../source/hevc/cutree_stream_ctrl.c
SRC_ENC_COMMON += ../source/hevc/cutreeasiccontroller.c
SRC_ENC_COMMON += ../source/hevc/sw_cu_tree.c
endif

ifeq ($(SUPPORT_CACHE),y)
SRC_ENC_COMMON += ../source/hevc/hevcenccache.c
endif

ifeq ($(SEI_BUILD_SUPPORT),y)
SRC_ENC_COMMON += ../source/hevc/hevcSei.c
endif

ifeq ($(CUINFO_BUILD_SUPPORT),y)
SRC_ENC_COMMON += ../source/hevc/vcenc_cuinfo.c
endif

ifeq ($(ROI_BUILD_SUPPORT),y)
SRC_ENC_COMMON += ../source/hevc/vcenc_fillroimap.c
endif

ifeq ($(STREAM_CTRL_BUILD_SUPPORT),y)
SRC_ENC_COMMON += ../source/hevc/vcenc_stream_ctrl.c
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


SRC_TRACE = debug_trace/enctrace.c

#test_data.c

# Source files for test case specific test IDs, compiler flag INTERNAL_TEST
# If these are not included some tests will fail


ifeq ($(INTERNAL_TEST),y)
SRC_TESTING = ../source/hevc/sw_test_id.c
endif

SRC_EWL_PC := ewl_x280_file.c
SRC_EWL_ARM = ewl/ewl.c
SRC_EWL_FreeRTOS = ewl.c


ifeq ($(VCMD_BUILD_SUPPORT),y)
    SRC_EWL_ARM += ewl/cwl_vc8000_vcmd_common.c
    SRC_EWL_FreeRTOS += cwl_vc8000_vcmd_common.c
endif

ifeq ($(LINUX_LOCK_BUILD_SUPPORT),y)
    SRC_EWL_ARM += ewl/ewl_linux_lock.c
endif

ifeq ($(SUPPORT_MEM_SYNC),y)
    SRC_EWL_ARM += ewl/ewl_memsync.c
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

$(warning $(VCENC_PATH))
LOCAL_SRC_FILES := ../source/common/encswhwregisters.c \
                   ewl/ewl_common.c \
                   ../source/common/buffer_info.c

# common parts only for encoder and not for video stab
INCLUDE_ENC_COMMON=y


# Combine the lists of all the source files included in the library build
ifeq ($(INCLUDE_HEVC),y)
    LOCAL_SRC_FILES += $(SRC_HEVC)
endif
ifeq ($(INCLUDE_AV1),y)
    LOCAL_SRC_FILES += $(SRC_AV1)
endif
ifeq ($(INCLUDE_VP9),y)
    LOCAL_SRC_FILES += $(SRC_VP9)
endif
ifeq ($(INCLUDE_JPEG),y)
    LOCAL_SRC_FILES += $(SRC_JPEG)
endif
ifeq ($(INCLUDE_VS),y)
    LOCAL_SRC_FILES += $(SRC_VS)
    CFLAGS += -DVIDEOSTAB_ENABLED
endif
ifeq ($(INTERNAL_TEST),y)
    LOCAL_SRC_FILES += $(SRC_TESTING)
endif

# add common encoder files
ifeq ($(INCLUDE_ENC_COMMON),y)
    LOCAL_SRC_FILES += $(SRC_ENC_COMMON)
endif

# if tracing flags are defined we need to compile the tracing functions
ifeq ($(TRACE),y)
    LOCAL_SRC_FILES += $(SRC_TRACE)
endif

ifneq ($(target),)
  ifneq ($(target),default)
    CFLAGS += -DTARGET -D$(target)
  endif
endif

CFLAGS += -DEWLHWCFGFILE=\"hwcfg/ewl_hwcfg$(FEATURES).c\"

# choose EWL source, system model uses its own EWL
ifneq (,$(findstring pclinux, $(MAKECMDGOALS)))
    LOCAL_SRC_FILES += $(SRC_EWL_SYS)
    INCLUDE += -I../../system/models/inc -Iewl -Iewl/system
    EWL_DIR = system
else
ifneq (,$(findstring system, $(MAKECMDGOALS)))
    LOCAL_SRC_FILES += $(SRC_EWL_SYS)
    INCLUDE += -I../../system/models/inc -Iewl -Iewl/system
    EWL_DIR = system
else
ifneq (,$(findstring testdata, $(MAKECMDGOALS)))
    LOCAL_SRC_FILES += $(SRC_EWL_SYS)
    INCLUDE += -I../../system/models/inc -Iewl -Iewl/system
    EWL_DIR = system
else
ifneq (,$(findstring eval, $(MAKECMDGOALS)))
    LOCAL_SRC_FILES += $(SRC_EWL_SYS)
    INCLUDE += -I../../system/models/inc -Iewl -Iewl/system
    EWL_DIR = system
else
ifeq (freertos_lib, $(findstring freertos_lib, $(MAKECMDGOALS)))
    LOCAL_SRC_FILES += $(SRC_EWL_FreeRTOS)
    INCLUDE += -Iewl
    EWL_DIR =
else
ifeq (freertos, $(findstring freertos, $(MAKECMDGOALS)))
    LOCAL_SRC_FILES += $(SRC_EWL_FreeRTOS)
    INCLUDE += -Iewl
    EWL_DIR =
else
    LOCAL_SRC_FILES += $(SRC_EWL_ARM)
    LOCAL_C_INCLUDES += -Iewl
    EWL_DIR =
endif
endif
endif
endif
endif
endif

LOCAL_SHARED_LIBRARIES += libcutils \
    liblog \
    libutils

LOCAL_CFLAGS := $(CFLAGS)
LOCAL_CFLAGS += -Wno-parentheses-equality -Wno-self-assign
$(warning $(LOCAL_SRC_FILES))
$(warning $(LOCAL_CFLAGS))

LOCAL_CFLAGS += -Wno-absolute-value
include $(BUILD_SHARED_LIBRARY)
include $(call all-makefiles-under,$(LOCAL_PATH))
