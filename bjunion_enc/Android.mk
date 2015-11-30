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
	enc/m8_enc/noise_reduction.cpp \
	decoder/decoder.c \
	decoder/amlv4l.c \
	decoder/amvideo.c

LOCAL_SRC_FILES += enc/intra_search/pred.cpp \
	enc/intra_search/pred_neon_asm.s

ifneq (,$(wildcard vendor/amlogic/frameworks/av/LibPlayer))
LIBPLAYER_DIR:=$(TOP)/vendor/amlogic/frameworks/av/LibPlayer
else
LIBPLAYER_DIR:=$(TOP)/packages/amlogic/LibPlayer
endif

LOCAL_STATIC_LIBRARIES := libamcodec libamadec libamavutils
LOCAL_SHARED_LIBRARIES  += libutils \
						libmedia \
						libdl \
						libcutils \
						libamsubdec \
						libbinder \
						libsystemwriteservice\
						libion


LOCAL_C_INCLUDES := $(LOCAL_PATH)/include \
		 $(LIBPLAYER_DIR)/amcodec/include \
		 $(TOP)/hardware/amlogic/gralloc \
		 $(LOCAL_PATH)/decoder

LOCAL_ARM_MODE := arm
LOCAL_MODULE:= libvpcodec
LOCAL_MODULE_TAGS := optional
LOCAL_PRELINK_MODULE := false
include $(BUILD_SHARED_LIBRARY)

