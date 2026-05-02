LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= test.c

LOCAL_SRC_FILES += tahoe_enc/vdi.c \
                   tahoe_enc/enc_driver.c \
                   tahoe_enc/libvptahoe_codec.c \
                   tahoe_enc/libvptahoe_wrapper.c

LOCAL_SHARED_LIBRARIES :=  libion \
                           libstagefright_foundation
LOCAL_SHARED_LIBRARIES +=  libcutils \
                           libutils  \
                           libion \
                           liblog

LOCAL_C_INCLUDES := $(TOP)/frameworks/av/media/libstagefright/foundation/include \
                    $(TOP)/system/memory/libion \
                    $(TOP)/system/memory/libion/include \
                    $(TOP)/system/memory/libion/kernel-headers \
                    $(TOP)/system/memory/include/ion \
                    $(LOCAL_PATH)/tahoe_enc/include \
		    $(LOCAL_PATH)/tahoe_enc
LOCAL_CFLAGS += -Wno-multichar -Wno-unused -DVENDOR_FW


LOCAL_MODULE_TAGS := optional

LOCAL_MODULE:= tahoe_test

LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0 legacy_proprietary
LOCAL_LICENSE_CONDITIONS := notice proprietary by_exception_only

include $(BUILD_EXECUTABLE)

include $(call all-makefiles-under,$(LOCAL_PATH))
