# Copyright (C) 2008 The Android Open Source Project
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
# HAL module implemenation, not prelinked and stored in
# hw/<COPYPIX_HARDWARE_MODULE_ID>.<ro.product.board>.so

ifeq ($(TARGET_BOARD_PLATFORM),omap3)
include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE := false
LOCAL_SHARED_LIBRARIES := liblog libcutils libutils libbinder
# LOCAL_SHARED_LIBRARIES += libmirror
# LOCAL_C_INCLUDES := motorola/hal/hdmi/mirror/include
LOCAL_SRC_FILES := v4l2_utils.c MotOverlay.cpp
LOCAL_MODULE := overlay.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE_TAGS := eng
include $(BUILD_SHARED_LIBRARY)
endif

ifeq ($(TARGET_BOARD_PLATFORM),omap4)
include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_SHARED_LIBRARIES := liblog libcutils
LOCAL_SRC_FILES := v4l2_utils.c TIOverlay.cpp
LOCAL_CFLAGS := -DTARGET_OMAP4
LOCAL_MODULE := overlay.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_TAGS:= optional
include $(BUILD_SHARED_LIBRARY)
endif

ifeq (0,1)
include $(CLEAR_VARS)
LOCAL_CFLAGS := -mabi=aapcs-linux 
LOCAL_SHARED_LIBRARIES := liblog libcutils
LOCAL_SRC_FILES := v4l2_utils.c v4l2_test.c
LOCAL_MODULE := v4l2_test
include $(BUILD_EXECUTABLE)
endif


include $(CLEAR_VARS)
ifeq ($(TARGET_BOARD_PLATFORM),omap4)
LOCAL_CFLAGS := -DTARGET_OMAP4
endif
LOCAL_SHARED_LIBRARIES := liblog libbinder libcutils libhardware libutils libui libsurfaceflinger_client
LOCAL_SRC_FILES := TIOverlay_test.cpp
LOCAL_MODULE := overlay_test
LOCAL_MODULE_TAGS:= optional
include $(BUILD_EXECUTABLE)
