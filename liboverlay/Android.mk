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

# Motorola recent overlay (omap4 and omap3)
ifneq ($(BOARD_USES_MOTOROLA_OVERLAY),)

include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE := false
LOCAL_SHARED_LIBRARIES := liblog libcutils libutils

ifeq ($(TARGET_BOARD_PLATFORM),omap4)
	LOCAL_SHARED_LIBRARIES += libmirror libbinder
	LOCAL_C_INCLUDES := motorola/hal/hdmi/mirror/include
endif

LOCAL_SRC_FILES := Mot_v4l2_utils.c MotOverlay.cpp
LOCAL_MODULE := overlay.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_MODULE_TAGS := eng
include $(BUILD_SHARED_LIBRARY)

else # BOARD_USES_MOTOROLA_OVERLAY

# Simple overlay (RC1)
ifeq ($(TARGET_BOARD_PLATFORM),omap3)
include $(CLEAR_VARS)
LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_PATH := $(TARGET_OUT_SHARED_LIBRARIES)/hw
LOCAL_SHARED_LIBRARIES := liblog libcutils
LOCAL_SRC_FILES := v4l2_utils.c overlay.cpp
LOCAL_MODULE := overlay.omap3
LOCAL_MODULE_TAGS := optional
include $(BUILD_SHARED_LIBRARY)
endif

endif # BOARD_USES_MOTOROLA_OVERLAY