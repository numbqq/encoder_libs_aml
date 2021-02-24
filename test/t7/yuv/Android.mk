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

LOCAL_MODULE := nv21_480p.yuv
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_PATH_32 := $(TARGET_OUT_VENDOR)/bin
LOCAL_SRC_FILES_arm := nv21_480p.yuv
$(warning LOCAL_SRC_FILES_arm=$(LOCAL_SRC_FILES_arm))

LOCAL_CHECK_ELF_FILES := false
include $(BUILD_PREBUILT)

include $(call all-makefiles-under,$(LOCAL_PATH))
