/*
 * Copyright (C) 2026 The LineageOS Project
 *
 * HIDL 2.1 BiometricsFingerprint HAL service definition for Samsung Galaxy S5 3G (k3gxx).
 * Wraps VcsClientWrapper to eliminate ValidityService.apk and legacy sockets.
 */

#ifndef ANDROID_HARDWARE_BIOMETRICS_FINGERPRINT_V2_1_BIOMETRICSFINGERPRINT_UNIVERSAL5422_H
#define ANDROID_HARDWARE_BIOMETRICS_FINGERPRINT_V2_1_BIOMETRICSFINGERPRINT_UNIVERSAL5422_H

#include <log/log.h>
#include <android/log.h>
#include <hidl/MQDescriptor.h>
#include <hidl/Status.h>
#include <android/hardware/biometrics/fingerprint/2.1/IBiometricsFingerprint.h>
#include <android/hardware/biometrics/fingerprint/2.1/IBiometricsFingerprintClientCallback.h>
#include <android/hardware/biometrics/fingerprint/2.1/types.h>
#include <mutex>
#include <memory>
#include "../src/VcsClientWrapper.h"

namespace android {
namespace hardware {
namespace biometrics {
namespace fingerprint {
namespace V2_1 {
namespace implementation {

using ::android::hardware::biometrics::fingerprint::V2_1::IBiometricsFingerprint;
using ::android::hardware::biometrics::fingerprint::V2_1::IBiometricsFingerprintClientCallback;
using ::android::hardware::biometrics::fingerprint::V2_1::RequestStatus;
using ::android::hardware::biometrics::fingerprint::V2_1::FingerprintAcquiredInfo;
using ::android::hardware::biometrics::fingerprint::V2_1::FingerprintError;
using ::android::hardware::Return;
using ::android::hardware::Void;
using ::android::hardware::hidl_vec;
using ::android::hardware::hidl_string;
using ::android::hardware::hidl_array;
using ::android::sp;

class BiometricsFingerprint : public IBiometricsFingerprint, public ::vcs::IVcsClientCallback {
public:
    BiometricsFingerprint();
    virtual ~BiometricsFingerprint();

    // IBiometricsFingerprint methods
    Return<uint64_t> setNotify(const sp<IBiometricsFingerprintClientCallback>& clientCallback) override;
    Return<uint64_t> preEnroll() override;
    Return<RequestStatus> enroll(const hidl_array<uint8_t, 69>& hat, uint32_t gid, uint32_t timeoutSec) override;
    Return<RequestStatus> postEnroll() override;
    Return<uint64_t> getAuthenticatorId() override;
    Return<RequestStatus> cancel() override;
    Return<RequestStatus> enumerate() override;
    Return<RequestStatus> remove(uint32_t gid, uint32_t fid) override;
    Return<RequestStatus> setActiveGroup(uint32_t gid, const hidl_string& storePath) override;
    Return<RequestStatus> authenticate(uint64_t operationId, uint32_t gid) override;

    // IVcsClientCallback methods
    void onEnrollResult(uint32_t fid, uint32_t gid, uint32_t remaining) override;
    void onAcquired(int32_t acquired_info, int32_t vendor_code) override;
    void onAuthenticated(uint32_t fid, uint32_t gid, const uint8_t* hat, size_t hat_len) override;
    void onError(int32_t error, int32_t vendor_code) override;
    void onRemoved(uint32_t fid, uint32_t gid) override;
    void onEnumerate(uint32_t fid, uint32_t gid, uint32_t remaining) override;

private:
    std::mutex mClientCallbackMutex;
    sp<IBiometricsFingerprintClientCallback> mClientCallback;
    std::unique_ptr<::vcs::VcsClientWrapper> mWrapper;
    uint64_t mDeviceId;
};

}  // namespace implementation
}  // namespace V2_1
}  // namespace fingerprint
}  // namespace biometrics
}  // namespace hardware
}  // namespace android

#endif // ANDROID_HARDWARE_BIOMETRICS_FINGERPRINT_V2_1_BIOMETRICSFINGERPRINT_UNIVERSAL5422_H
