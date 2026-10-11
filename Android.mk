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

LOCAL_PATH := $(call my-dir)

define k3gxx-sensor-prebuilt
include $(CLEAR_VARS)
LOCAL_MODULE := $(1)
LOCAL_SRC_FILES := $(2)
LOCAL_MODULE_CLASS := SHARED_LIBRARIES
LOCAL_MODULE_SUFFIX := .so
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER := samsung
LOCAL_VENDOR_MODULE := true
LOCAL_MULTILIB := 32
LOCAL_CHECK_ELF_FILES := false
ifneq ($(strip $(3)),)
LOCAL_MODULE_RELATIVE_PATH := $(3)
endif
include $(BUILD_PREBUILT)
endef

define k3gxx-sensor-exec
include $(CLEAR_VARS)
LOCAL_MODULE := $(1)
LOCAL_SRC_FILES := $(2)
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_OWNER := samsung
LOCAL_VENDOR_MODULE := true
LOCAL_MULTILIB := 32
LOCAL_CHECK_ELF_FILES := false
include $(BUILD_PREBUILT)
endef

# Sensors (SensorHub)
$(eval $(call k3gxx-sensor-prebuilt,libsensorhub,proprietary/lib/libsensorhub.so,))
$(eval $(call k3gxx-sensor-prebuilt,libsensorhubservice,proprietary/lib/libsensorhubservice.so,))
$(eval $(call k3gxx-sensor-prebuilt,sensorhubs.universal5422,proprietary/lib/hw/sensorhubs.universal5422.so,hw))
$(eval $(call k3gxx-sensor-prebuilt,sensors.universal5422,proprietary/lib/hw/sensors.universal5422.so,hw))
$(eval $(call k3gxx-sensor-exec,sensorhubservice,proprietary/bin/sensorhubservice))

# Validity Fingerprint
$(eval $(call k3gxx-sensor-prebuilt,libfpasmtztransport,proprietary/lib/libfpasmtztransport.so,))
$(eval $(call k3gxx-sensor-prebuilt,libvalAuth,proprietary/lib/libvalAuth.so,))
$(eval $(call k3gxx-sensor-prebuilt,libvcsfp,proprietary/lib/libvcsfp.so,))
$(eval $(call k3gxx-sensor-prebuilt,libvfmClient,proprietary/lib/libvfmClient.so,))
$(eval $(call k3gxx-sensor-prebuilt,libvfmtztransport,proprietary/lib/libvfmtztransport.so,))
$(eval $(call k3gxx-sensor-exec,vcsFPService,proprietary/bin/vcsFPService))

# Fingerprint open-source HAL and HIDL service
include $(LOCAL_PATH)/hal/Android.mk
include $(LOCAL_PATH)/service/Android.mk
