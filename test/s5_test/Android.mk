LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/bin
#LOCAL_MODULE_PATH_64 := $(TARGET_OUT_VENDOR)/bin64
endif

# the path where to find header files

# list of used sourcefiles
LOCAL_SRC_FILES := s5_encoder_test.c

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE:= s5_encoder_test

LOCAL_LICENSE_KINDS := SPDX-license-identifier-Apache-2.0
LOCAL_LICENSE_CONDITIONS := notice

include $(BUILD_EXECUTABLE)

include $(call all-makefiles-under,$(LOCAL_PATH))
