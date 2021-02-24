LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/lib
LOCAL_MODULE_PATH_64 := $(TARGET_OUT_VENDOR)/lib64
endif

LOCAL_LDLIBS    := -lm -llog
LOCAL_CFLAGS += -Wno-multichar -Wno-unused -Wno-unused-parameter
LOCAL_SRC_FILES := \
    jpegenc.c \
	jpegenc_api.c

LOCAL_SHARED_LIBRARIES  += libcutils \
	libutils  \
	libion \
	liblog

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include \
	$(TOP)/hardware/amlogic/gralloc \
	$(TOP)/system/core/include
	#$(LIBPLAYER_DIR)/amcodec/include

LOCAL_CFLAGS += -Wno-multichar -Wno-error

LOCAL_ARM_MODE := arm
LOCAL_MODULE:= libjpegenc_api
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)
