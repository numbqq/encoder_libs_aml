LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= test.cpp

LOCAL_SHARED_LIBRARIES := \
		libstagefright_foundation \
        libvpcodec

LOCAL_C_INCLUDE+= $(TOP)/frameworks/av/media/libstagefright/foundation/include
LOCAL_CFLAGS += -Wno-multichar -Wno-unused

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE:= testEncApi

include $(BUILD_EXECUTABLE)

include $(call all-makefiles-under,$(LOCAL_PATH))
