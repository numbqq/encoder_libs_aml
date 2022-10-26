LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/lib
LOCAL_MODULE_PATH_64 := $(TARGET_OUT_VENDOR)/lib64
endif

HEVC ?= y

VCENC_PATH := $(LOCAL_PATH)/../

include $(VCENC_PATH)/build/globaldefs.mk
include $(VCENC_PATH)/build/globalrules.mk


# the path where to find header files
#INCFLAGS = -I../inc -I../source/common -I../source/hevc
LOCAL_C_INCLUDES        := \
                           $(VCENC_PATH)/source/hevc \
                           $(VCENC_PATH)/inc \
                           $(VCENC_PATH)/source/common
ifneq ($(target),)
  ifneq ($(target), default)
    CFLAGS += -DTARGET -D$(target)
  endif
endif

# list of used sourcefiles
LOCAL_SRC_FILES := test_bench.c test_bench_utils.c get_option.c

#LIB = ../linux_reference/libh2enc.a
LOCAL_SHARED_LIBRARIES := libcutils \
    liblog \
    libutils \
    libvcenc

# name of the output executable
#TARGET = vc_test_enc

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE:= libvc_codec

include $(BUILD_SHARED_LIBRARY)


include $(CLEAR_VARS)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/bin
#LOCAL_MODULE_PATH_64 := $(TARGET_OUT_VENDOR)/bin64
endif

# the path where to find header files
#INCFLAGS = -I../inc -I../source/common -I../source/hevc
LOCAL_C_INCLUDES        :=  vp_vc_codec_1_0.h

# list of used sourcefiles
LOCAL_SRC_FILES := test.c

LOCAL_SHARED_LIBRARIES := libvc_codec

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE:= vc_test_enc

include $(BUILD_EXECUTABLE)
