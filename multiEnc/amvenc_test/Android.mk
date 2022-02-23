# Copyright 2013 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
ifeq ($(shell test $(PLATFORM_SDK_VERSION) -ge 26 && echo OK),OK)
LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/bin
#LOCAL_MODULE_PATH_64 := $(TARGET_OUT_VENDOR)/lib64
endif
LOCAL_SRC_FILES := test.c test_dma.c IONmem.c

LOCAL_SHARED_LIBRARIES := libion \
	  libmedia_omx libutils libbinder libstagefright_foundation \
	libjpeg   libcutils liblog libEGL libGLESv2 libamvenc_api libvpcodec

LOCAL_C_INCLUDES := \
	frameworks/av/media/libstagefright \
	frameworks/av/media/libstagefright/include \
	frameworks/native/include/media/openmax \
	external/jpeg \
	$(LOCAL_PATH) \
	$(LOCAL_PATH)/include \
	$(LOCAL_PATH)/../vpuapi/include \
	$(LOCAL_PATH)/../vpuapi

LOCAL_CFLAGS := -Wall -Wno-unused-variable -Wno-sign-compare
LOCAL_CFLAGS += -Wno-multichar -Wno-sometimes-uninitialized -Wno-format
#LOCAL_CFLAGS += -UNDEBUG

LOCAL_MODULE_TAGS := optional

LOCAL_MODULE:= aml_enc_test

include $(BUILD_EXECUTABLE)
