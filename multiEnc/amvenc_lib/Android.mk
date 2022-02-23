LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/lib
LOCAL_MODULE_PATH_64 := $(TARGET_OUT_VENDOR)/lib64
endif

LOCAL_CFLAGS := \
	-fPIC -D_POSIX_SOURCE -D_FILE_OFFSET_BITS=64

LOCAL_LDLIBS := -lm -llog

LOCAL_C_INCLUDES:= \
	$(LOCAL_PATH)/ \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/../vpuapi/include \
	$(TOP)/vendor/amlogic/common/system/libge2d/v1/include \
	$(LOCAL_PATH)/../vpuapi

LOCAL_SRC_FILES := \
	libvpmulti_codec.c AML_MultiEncoder.c

LOCAL_ARM_MODE := arm
LOCAL_SHARED_LIBRARIES += libutils libcutils libamvenc_api libge2d
#LOCAL_STATIC_LIBRARIES += libge2d-2.0-static

LOCAL_EXPORT_C_INCLUDE_DIRS := $(LOCAL_PATH)/include

LOCAL_CFLAGS=-Wno-error
LOCAL_MODULE := libvpcodec
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional

include $(BUILD_SHARED_LIBRARY)
