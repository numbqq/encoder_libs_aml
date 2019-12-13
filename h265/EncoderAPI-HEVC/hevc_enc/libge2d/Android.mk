LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_LDLIBS    := -lm -llog

LOCAL_CFLAGS += -Wno-multichar -Wno-unused -Wno-unused-parameter

LOCAL_SRC_FILES := \
	aml_ge2d.c \
	ge2d_port.c \
	ion.c \
	IONmem.c \
	ge2d_feature_test.c

LOCAL_SHARED_LIBRARIES  += \
	libcutils \
	libutils  \
	libion \
	liblog


LOCAL_C_INCLUDES := \
	$(LOCAL_PATH)/include \
	$(TOP)/hardware/amlogic/gralloc \
	$(TOP)/system/core/include

LOCAL_CFLAGS += -Wno-multichar -Wno-error

LOCAL_ARM_MODE := arm
LOCAL_MODULE:= libge2d
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false
include $(BUILD_STATIC_LIBRARY)
