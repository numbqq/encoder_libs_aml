LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_LDLIBS    := -lm -llog
LOCAL_CFLAGS += -Wno-multichar -Wno-unused -Wno-unused-parameter
LOCAL_SRC_FILES := \
    libvptahoe_wrapper.c \
    libvptahoe_codec.c  \
    enc_driver.c \
    vdi.c

LOCAL_SHARED_LIBRARIES  += libcutils \
	libutils  \
	libion \
	liblog


LOCAL_C_INCLUDES := $(LOCAL_PATH)/include \
	            $(LOCAL_PATH)

LOCAL_CFLAGS += -Wno-multichar -Wno-error -DVENDOR_FW

LOCAL_ARM_MODE := arm
LOCAL_MODULE:= libamvenc_tahoe
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false


LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0 legacy_proprietary
LOCAL_LICENSE_CONDITIONS := notice proprietary by_exception_only

include $(BUILD_SHARED_LIBRARY)
