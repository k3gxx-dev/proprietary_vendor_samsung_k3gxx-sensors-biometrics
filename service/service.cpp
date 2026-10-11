/*
 * Copyright (C) 2026 The LineageOS Project
 *
 * Daemon main entrypoint for android.hardware.biometrics.fingerprint@2.1-service.universal5422.
 */

#define LOG_TAG "android.hardware.biometrics.fingerprint@2.1-service.universal5422"

#include <android/hardware/biometrics/fingerprint/2.1/IBiometricsFingerprint.h>
#include <hidl/HidlTransportSupport.h>
#include <log/log.h>
#include "BiometricsFingerprint.h"

using android::hardware::biometrics::fingerprint::V2_1::IBiometricsFingerprint;
using android::hardware::biometrics::fingerprint::V2_1::implementation::BiometricsFingerprint;
using android::hardware::configureRpcThreadpool;
using android::hardware::joinRpcThreadpool;
using android::sp;
using android::status_t;
using android::OK;

int main() {
    ALOGI("Starting android.hardware.biometrics.fingerprint@2.1-service.universal5422...");

    sp<IBiometricsFingerprint> bio = new BiometricsFingerprint();
    if (bio == nullptr) {
        ALOGE("Failed to create BiometricsFingerprint instance");
        return 1;
    }

    configureRpcThreadpool(1, true /*callerWillJoin*/);

    status_t status = bio->registerAsService();
    if (status != OK) {
        ALOGE("Failed to register BiometricsFingerprint HIDL service (status %d)", status);
        return 1;
    }

    ALOGI("BiometricsFingerprint HIDL service successfully registered and waiting for requests");
    joinRpcThreadpool();
    return 0;
}
