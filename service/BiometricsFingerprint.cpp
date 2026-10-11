/*
 * Copyright (C) 2026 The LineageOS Project
 *
 * HIDL 2.1 BiometricsFingerprint HAL implementation for Samsung Galaxy S5 3G (k3gxx).
 */

#define LOG_TAG "BiometricsFingerprintHal"
#define LOG_NDEBUG 0

#include <inttypes.h>
#include <unistd.h>
#include <log/log.h>
#include "BiometricsFingerprint.h"

namespace android {
namespace hardware {
namespace biometrics {
namespace fingerprint {
namespace V2_1 {
namespace implementation {

BiometricsFingerprint::BiometricsFingerprint()
    : mDeviceId(1) {
    ALOGI("Instantiating BiometricsFingerprint HIDL 2.1 service for universal5422...");
    mWrapper = std::make_unique<::vcs::VcsClientWrapper>();
    mWrapper->setCallback(this);
    if (!mWrapper->init()) {
        ALOGW("Warning: Initial VcsClientWrapper init failed; will retry on setNotify / hardware calls");
    }
}

BiometricsFingerprint::~BiometricsFingerprint() {
    ALOGI("Destructing BiometricsFingerprint service");
    if (mWrapper != nullptr) {
        mWrapper->release();
    }
}

Return<uint64_t> BiometricsFingerprint::setNotify(
        const sp<IBiometricsFingerprintClientCallback>& clientCallback) {
    std::lock_guard<std::mutex> lock(mClientCallbackMutex);
    ALOGI("setNotify called with callback %p", clientCallback.get());
    mClientCallback = clientCallback;

    // Garante que o wrapper esteja inicializado
    if (mWrapper != nullptr) {
        mWrapper->init();
    }

    return mDeviceId;
}

Return<uint64_t> BiometricsFingerprint::preEnroll() {
    ALOGD("preEnroll called");
    if (mWrapper == nullptr) {
        return 0;
    }
    return mWrapper->preEnroll();
}

Return<RequestStatus> BiometricsFingerprint::enroll(
        const hidl_array<uint8_t, 69>& hat, uint32_t gid, uint32_t timeoutSec) {
    ALOGI("enroll called: gid=%u, timeout=%u", gid, timeoutSec);
    if (mWrapper == nullptr) {
        return RequestStatus::SYS_EINVAL;
    }

    int ret = mWrapper->enroll(hat.data(), 69, gid, timeoutSec);
    return (ret == 0) ? RequestStatus::SYS_OK : RequestStatus::SYS_EINVAL;
}

Return<RequestStatus> BiometricsFingerprint::postEnroll() {
    ALOGD("postEnroll called");
    if (mWrapper == nullptr) {
        return RequestStatus::SYS_EINVAL;
    }
    mWrapper->postEnroll();
    return RequestStatus::SYS_OK;
}

Return<uint64_t> BiometricsFingerprint::getAuthenticatorId() {
    ALOGD("getAuthenticatorId called");
    if (mWrapper == nullptr) {
        return 0;
    }
    return mWrapper->getAuthenticatorId();
}

Return<RequestStatus> BiometricsFingerprint::cancel() {
    ALOGI("cancel called");
    if (mWrapper == nullptr) {
        return RequestStatus::SYS_EINVAL;
    }
    mWrapper->cancel();
    return RequestStatus::SYS_OK;
}

Return<RequestStatus> BiometricsFingerprint::enumerate() {
    ALOGI("enumerate called");
    if (mWrapper == nullptr) {
        return RequestStatus::SYS_EINVAL;
    }
    mWrapper->enumerate();
    return RequestStatus::SYS_OK;
}

Return<RequestStatus> BiometricsFingerprint::remove(uint32_t gid, uint32_t fid) {
    ALOGI("remove called: gid=%u, fid=%u", gid, fid);
    if (mWrapper == nullptr) {
        return RequestStatus::SYS_EINVAL;
    }
    int ret = mWrapper->remove(gid, fid);
    return (ret == 0) ? RequestStatus::SYS_OK : RequestStatus::SYS_EINVAL;
}

Return<RequestStatus> BiometricsFingerprint::setActiveGroup(
        uint32_t gid, const hidl_string& storePath) {
    ALOGI("setActiveGroup called: gid=%u, storePath=%s", gid, storePath.c_str());
    if (mWrapper == nullptr) {
        return RequestStatus::SYS_EINVAL;
    }
    mWrapper->setActiveGroup(gid, storePath.c_str());
    return RequestStatus::SYS_OK;
}

Return<RequestStatus> BiometricsFingerprint::authenticate(
        uint64_t operationId, uint32_t gid) {
    ALOGI("authenticate called: opId=%" PRIu64 ", gid=%u", operationId, gid);
    if (mWrapper == nullptr) {
        return RequestStatus::SYS_EINVAL;
    }
    int ret = mWrapper->authenticate(operationId, gid);
    return (ret == 0) ? RequestStatus::SYS_OK : RequestStatus::SYS_EINVAL;
}

// ============================================================================
// IVcsClientCallback: Repasse de eventos para a interface HIDL
// ============================================================================

void BiometricsFingerprint::onEnrollResult(uint32_t fid, uint32_t gid, uint32_t remaining) {
    std::lock_guard<std::mutex> lock(mClientCallbackMutex);
    if (mClientCallback != nullptr) {
        ALOGD("Forwarding onEnrollResult: fid=%u, gid=%u, remaining=%u", fid, gid, remaining);
        Return<void> ret = mClientCallback->onEnrollResult(mDeviceId, fid, gid, remaining);
        if (!ret.isOk()) {
            ALOGE("Failed to forward onEnrollResult: %s", ret.description().c_str());
        }
    }
}

void BiometricsFingerprint::onAcquired(int32_t acquired_info, int32_t vendor_code) {
    std::lock_guard<std::mutex> lock(mClientCallbackMutex);
    if (mClientCallback != nullptr) {
        ALOGD("Forwarding onAcquired: info=%d, vendor=%d", acquired_info, vendor_code);
        Return<void> ret = mClientCallback->onAcquired(
            mDeviceId, static_cast<FingerprintAcquiredInfo>(acquired_info), vendor_code);
        if (!ret.isOk()) {
            ALOGE("Failed to forward onAcquired: %s", ret.description().c_str());
        }
    }
}

void BiometricsFingerprint::onAuthenticated(
        uint32_t fid, uint32_t gid, const uint8_t* hat, size_t hat_len) {
    std::lock_guard<std::mutex> lock(mClientCallbackMutex);
    if (mClientCallback != nullptr) {
        ALOGI("Forwarding onAuthenticated: fid=%u, gid=%u, hat_len=%zu", fid, gid, hat_len);
        hidl_vec<uint8_t> token;
        if (hat != nullptr && hat_len > 0) {
            token.resize(hat_len);
            memcpy(token.data(), hat, hat_len);
        }
        Return<void> ret = mClientCallback->onAuthenticated(mDeviceId, fid, gid, token);
        if (!ret.isOk()) {
            ALOGE("Failed to forward onAuthenticated: %s", ret.description().c_str());
        }
    }
}

void BiometricsFingerprint::onError(int32_t error, int32_t vendor_code) {
    std::lock_guard<std::mutex> lock(mClientCallbackMutex);
    if (mClientCallback != nullptr) {
        ALOGI("Forwarding onError: error=%d, vendor=%d", error, vendor_code);
        Return<void> ret = mClientCallback->onError(
            mDeviceId, static_cast<FingerprintError>(error), vendor_code);
        if (!ret.isOk()) {
            ALOGE("Failed to forward onError: %s", ret.description().c_str());
        }
    }
}

void BiometricsFingerprint::onRemoved(uint32_t fid, uint32_t gid) {
    std::lock_guard<std::mutex> lock(mClientCallbackMutex);
    if (mClientCallback != nullptr) {
        ALOGI("Forwarding onRemoved: fid=%u, gid=%u", fid, gid);
        Return<void> ret = mClientCallback->onRemoved(mDeviceId, fid, gid, 0);
        if (!ret.isOk()) {
            ALOGE("Failed to forward onRemoved: %s", ret.description().c_str());
        }
    }
}

void BiometricsFingerprint::onEnumerate(uint32_t fid, uint32_t gid, uint32_t remaining) {
    std::lock_guard<std::mutex> lock(mClientCallbackMutex);
    if (mClientCallback != nullptr) {
        ALOGD("Forwarding onEnumerate: fid=%u, gid=%u, remaining=%u", fid, gid, remaining);
        Return<void> ret = mClientCallback->onEnumerate(mDeviceId, fid, gid, remaining);
        if (!ret.isOk()) {
            ALOGE("Failed to forward onEnumerate: %s", ret.description().c_str());
        }
    }
}

}  // namespace implementation
}  // namespace V2_1
}  // namespace fingerprint
}  // namespace biometrics
}  // namespace hardware
}  // namespace android
