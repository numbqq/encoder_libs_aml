LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_LDLIBS := -lm -llog

LOCAL_SRC_FILES := \
	vdi.c \
	vdi_osal.c \
	debug.c

LOCAL_SRC_FILES += \
	vpuapi.c \
	product.c \
	wave/wave5.c \
	vpuapifunc.c

#define MAKEANDROID

LOCAL_SHARED_LIBRARIES += libcutils \
			libutils \
			libion \
			libdl \
			liblog

LOCAL_C_INCLUDES := $(LOCAL_PATH) \
		$(LOCAL_PATH)/wave \
		$(LOCAL_PATH)/include


LOCAL_ARM_MODE := arm
LOCAL_MODULE:= libamvenc_api
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)
