#
# Copyright (C) 2016 The CyanogenMod Project
# Copyright (C) 2016 The Mokee Project
# Copyright (C) 2026 The LineageOS Project
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
#

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := fingerprint.universal5422
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_VENDOR_MODULE := true
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
    fingerprint.cpp \
    ../src/VcsClientWrapper.cpp \
    ../src/GestureEngine.cpp

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../include \
    $(LOCAL_PATH)/../src

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libcutils \
    libhardware \
    libutils \
    libvfmClient

LOCAL_CFLAGS := -Wall -Werror -Wno-unused-parameter

include $(BUILD_SHARED_LIBRARY)