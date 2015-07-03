LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
    libvpcodec.cpp

LOCAL_SRC_FILES += AML_HWEncoder.cpp \
	enc_api.cpp \
	enc/common/fill_buffer.cpp \
	enc/m8_enc_fast/rate_control_m8_fast.cpp \
	enc/m8_enc_fast/m8venclib_fast.cpp \
	enc/m8_enc/dump.cpp \
	enc/m8_enc/m8venclib.cpp \
	enc/m8_enc/rate_control_m8.cpp \
	enc/m8_enc/noise_reduction.cpp

LOCAL_SRC_FILES += enc/intra_search/pred.cpp \
	enc/intra_search/pred_neon_asm.s

LOCAL_SHARED_LIBRARIES += libcutils #libutils

#LOCAL_SHARED_LIBRARIES += libbinder

LOCAL_C_INCLUDES := $(LOCAL_PATH)/include \
#		 $(TOP)/frameworks/native/services \
#		 $(TOP)/frameworks/native/include

LOCAL_ARM_MODE := arm
LOCAL_MODULE:= libvpcodec
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)

