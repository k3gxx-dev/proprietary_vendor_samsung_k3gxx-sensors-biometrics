/*
 * Copyright (C) 2014 The Android Open Source Project
 * Copyright (C) 2016 The CyanogenMod Project
 * Copyright (C) 2016 The Mokee Project
 * Copyright (C) 2026 The LineageOS Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "FingerprintHal_universal5422"
#define LOG_NDEBUG 0

#include <errno.h>
#include <malloc.h>
#include <string.h>
#include <unistd.h>
#include <log/log.h>
#include <hardware/hardware.h>
#include <hardware/fingerprint.h>

#include "VcsClientWrapper.h"

namespace {

class HalCallbackForwarder : public vcs::IVcsClientCallback {
public:
    HalCallbackForwarder(fingerprint_device_t* dev, pthread_mutex_t* lock)
        : mDev(dev), mLock(lock) {}

    void onEnrollResult(uint32_t fid, uint32_t /*gid*/, uint32_t remaining) override {
        ALOGD("HalCallbackForwarder::onEnrollResult: fid=%u, remaining=%u", fid, remaining);
        fingerprint_msg_t msg = {};
        msg.type = FINGERPRINT_TEMPLATE_ENROLLING;
        msg.data.enroll.finger.fid = fid;
        msg.data.enroll.samples_remaining = remaining;
        notifyCaller(&msg);
    }

    void onAcquired(int32_t acquired_info, int32_t /*vendor_code*/) override {
        ALOGD("HalCallbackForwarder::onAcquired: info=%d", acquired_info);
        fingerprint_msg_t msg = {};
        msg.type = FINGERPRINT_ACQUIRED;
        msg.data.acquired.acquired_info = static_cast<fingerprint_acquired_info_t>(acquired_info);
        notifyCaller(&msg);
    }

    void onAuthenticated(uint32_t fid, uint32_t /*gid*/, const uint8_t* hat, size_t hat_len) override {
        ALOGI("HalCallbackForwarder::onAuthenticated: fid=%u", fid);
        fingerprint_msg_t msg = {};
        msg.type = FINGERPRINT_AUTHENTICATED;
        msg.data.authenticated.finger.fid = fid;
        msg.data.authenticated.finger.gid = 0;
        if (hat != nullptr && hat_len >= sizeof(hw_auth_token_t)) {
            memcpy(&msg.data.authenticated.hat, hat, sizeof(hw_auth_token_t));
        }
        notifyCaller(&msg);
    }

    void onError(int32_t error, int32_t /*vendor_code*/) override {
        ALOGE("HalCallbackForwarder::onError: error=%d", error);
        fingerprint_msg_t msg = {};
        msg.type = FINGERPRINT_ERROR;
        msg.data.error = static_cast<fingerprint_error_t>(error);
        notifyCaller(&msg);
    }

    void onRemoved(uint32_t fid, uint32_t gid) override {
        ALOGI("HalCallbackForwarder::onRemoved: fid=%u, gid=%u", fid, gid);
        fingerprint_msg_t msg = {};
        msg.type = FINGERPRINT_TEMPLATE_REMOVED;
        msg.data.removed.finger.fid = fid;
        msg.data.removed.finger.gid = gid;
        msg.data.removed.remaining_templates = 0;
        notifyCaller(&msg);
    }

    void onEnumerate(uint32_t fid, uint32_t gid, uint32_t remaining) override {
        ALOGD("HalCallbackForwarder::onEnumerate: fid=%u, gid=%u, remaining=%u", fid, gid, remaining);
        fingerprint_msg_t msg = {};
        msg.type = FINGERPRINT_TEMPLATE_ENUMERATING;
        msg.data.enumerated.finger.fid = fid;
        msg.data.enumerated.finger.gid = gid;
        msg.data.enumerated.remaining_templates = remaining;
        notifyCaller(&msg);
    }

private:
    void notifyCaller(fingerprint_msg_t* msg) {
        pthread_mutex_lock(mLock);
        if (mDev != nullptr && mDev->notify != nullptr) {
            mDev->notify(msg);
        }
        pthread_mutex_unlock(mLock);
    }

    fingerprint_device_t* mDev;
    pthread_mutex_t* mLock;
};

struct vcs_fingerprint_device_impl_t {
    fingerprint_device_t device;
    vcs::VcsClientWrapper* wrapper;
    HalCallbackForwarder* forwarder;
    pthread_mutex_t lock;
};

uint64_t fingerprint_pre_enroll(struct fingerprint_device* device) {
    vcs_fingerprint_device_impl_t* vdev = reinterpret_cast<vcs_fingerprint_device_impl_t*>(device);
    return vdev->wrapper->preEnroll();
}

int fingerprint_post_enroll(struct fingerprint_device* device) {
    vcs_fingerprint_device_impl_t* vdev = reinterpret_cast<vcs_fingerprint_device_impl_t*>(device);
    return vdev->wrapper->postEnroll();
}

uint64_t fingerprint_get_auth_id(struct fingerprint_device* device) {
    vcs_fingerprint_device_impl_t* vdev = reinterpret_cast<vcs_fingerprint_device_impl_t*>(device);
    return vdev->wrapper->getAuthenticatorId();
}

int fingerprint_set_active_group(struct fingerprint_device* device, uint32_t gid, const char* path) {
    vcs_fingerprint_device_impl_t* vdev = reinterpret_cast<vcs_fingerprint_device_impl_t*>(device);
    return vdev->wrapper->setActiveGroup(gid, path);
}

int fingerprint_authenticate(struct fingerprint_device* device, uint64_t operation_id, uint32_t gid) {
    vcs_fingerprint_device_impl_t* vdev = reinterpret_cast<vcs_fingerprint_device_impl_t*>(device);
    return vdev->wrapper->authenticate(operation_id, gid);
}

int fingerprint_enroll(struct fingerprint_device* device, const hw_auth_token_t* hat,
                       uint32_t gid, uint32_t timeout_sec) {
    vcs_fingerprint_device_impl_t* vdev = reinterpret_cast<vcs_fingerprint_device_impl_t*>(device);
    return vdev->wrapper->enroll(reinterpret_cast<const uint8_t*>(hat), sizeof(hw_auth_token_t), gid, timeout_sec);
}

int fingerprint_cancel(struct fingerprint_device* device) {
    vcs_fingerprint_device_impl_t* vdev = reinterpret_cast<vcs_fingerprint_device_impl_t*>(device);
    return vdev->wrapper->cancel();
}

int fingerprint_enumerate(struct fingerprint_device* device) {
    if (device == nullptr) return -EINVAL;
    vcs_fingerprint_device_impl_t* vdev = reinterpret_cast<vcs_fingerprint_device_impl_t*>(device);
    return vdev->wrapper->enumerate();
}

int fingerprint_remove(struct fingerprint_device* device, uint32_t gid, uint32_t fid) {
    vcs_fingerprint_device_impl_t* vdev = reinterpret_cast<vcs_fingerprint_device_impl_t*>(device);
    return vdev->wrapper->remove(gid, fid);
}

int set_notify_callback(struct fingerprint_device* device, fingerprint_notify_t notify) {
    vcs_fingerprint_device_impl_t* vdev = reinterpret_cast<vcs_fingerprint_device_impl_t*>(device);
    pthread_mutex_lock(&vdev->lock);
    device->notify = notify;
    pthread_mutex_unlock(&vdev->lock);
    ALOGD("Fingerprint notify callback updated");
    return 0;
}

int fingerprint_close(hw_device_t* device) {
    if (device == nullptr) return -EINVAL;
    vcs_fingerprint_device_impl_t* vdev = reinterpret_cast<vcs_fingerprint_device_impl_t*>(device);
    vdev->wrapper->release();
    delete vdev->forwarder;
    delete vdev->wrapper;
    pthread_mutex_destroy(&vdev->lock);
    free(vdev);
    ALOGI("Fingerprint HAL closed");
    return 0;
}

int fingerprint_open(const hw_module_t* module, const char* /*id*/, hw_device_t** device) {
    if (device == nullptr) return -EINVAL;

    ALOGI("Opening modern native Validity fingerprint HAL (universal5422)...");
    vcs_fingerprint_device_impl_t* vdev =
        reinterpret_cast<vcs_fingerprint_device_impl_t*>(calloc(1, sizeof(vcs_fingerprint_device_impl_t)));
    if (vdev == nullptr) return -ENOMEM;

    pthread_mutex_init(&vdev->lock, nullptr);

    vdev->device.common.tag = HARDWARE_DEVICE_TAG;
    vdev->device.common.version = HARDWARE_MODULE_API_VERSION(2, 0);
    vdev->device.common.module = const_cast<hw_module_t*>(module);
    vdev->device.common.close = fingerprint_close;

    vdev->device.pre_enroll = fingerprint_pre_enroll;
    vdev->device.enroll = fingerprint_enroll;
    vdev->device.post_enroll = fingerprint_post_enroll;
    vdev->device.get_authenticator_id = fingerprint_get_auth_id;
    vdev->device.set_active_group = fingerprint_set_active_group;
    vdev->device.authenticate = fingerprint_authenticate;
    vdev->device.cancel = fingerprint_cancel;
    vdev->device.enumerate = fingerprint_enumerate;
    vdev->device.remove = fingerprint_remove;
    vdev->device.set_notify = set_notify_callback;
    vdev->device.notify = nullptr;

    vdev->wrapper = new vcs::VcsClientWrapper();
    vdev->forwarder = new HalCallbackForwarder(&vdev->device, &vdev->lock);
    vdev->wrapper->setCallback(vdev->forwarder);

    if (!vdev->wrapper->init()) {
        ALOGW("VcsClientWrapper init delayed; will retry when service requests biometrics");
    }

    *device = &vdev->device.common;
    ALOGI("Fingerprint HAL opened successfully");
    return 0;
}

struct hw_module_methods_t fingerprint_module_methods = {
    .open = fingerprint_open,
};

} // namespace

fingerprint_module_t HAL_MODULE_INFO_SYM = {
    .common = {
        .tag                = HARDWARE_MODULE_TAG,
        .module_api_version = FINGERPRINT_MODULE_API_VERSION_2_0,
        .hal_api_version    = HARDWARE_HAL_API_VERSION,
        .id                 = FINGERPRINT_HARDWARE_MODULE_ID,
        .name               = "K3GXX Modern Native Fingerprint HAL",
        .author             = "LineageOS Project",
        .methods            = &fingerprint_module_methods,
        .dso                = nullptr,
        .reserved           = {0},
    },
};
