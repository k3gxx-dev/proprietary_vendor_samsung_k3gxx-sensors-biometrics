#
# Copyright (C) 2017 The Android Open Source Project
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

LOCAL_MODULE := android.hardware.biometrics.fingerprint@2.1-service.universal5422
LOCAL_INIT_RC := android.hardware.biometrics.fingerprint@2.1-service.universal5422.rc
LOCAL_VINTF_FRAGMENTS := android.hardware.biometrics.fingerprint@2.1-service.universal5422.xml
LOCAL_VENDOR_MODULE := true
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MULTILIB := 32
LOCAL_MODULE_TAGS := optional

LOCAL_SRC_FILES := \
    BiometricsFingerprint.cpp \
    service.cpp \
    ../src/VcsClientWrapper.cpp \
    ../src/GestureEngine.cpp

LOCAL_C_INCLUDES := \
    $(LOCAL_PATH)/../include \
    $(LOCAL_PATH)/../src

LOCAL_SHARED_LIBRARIES := \
    libcutils \
    liblog \
    libhidlbase \
    libhardware \
    libutils \
    android.hardware.biometrics.fingerprint@2.1 \
    libvfmClient

LOCAL_CFLAGS := -Wall -Werror -Wno-unused-parameter

include $(BUILD_EXECUTABLE)
