/*
 * Copyright (C) 2026 The LineageOS Project
 *
 * C++ abstraction wrapper implementation for Validity VFS61xx client.
 * Connects directly to libvfmClient.so and translates native swipe events
 * to Android Biometrics callbacks, eliminating ValidityService.apk and sockets.
 */

#define LOG_TAG "VcsClientWrapper"
#define LOG_NDEBUG 0

#include <errno.h>
#include <endian.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <algorithm>
#include <log/log.h>

#include "VcsClientWrapper.h"

#define MAX_NUM_FINGERS 5
#define CLIENT_PIPE_NAME "validity"

namespace vcs {

VcsClientWrapper::VcsClientWrapper()
    : mCallback(nullptr),
      mValidity(nullptr),
      mState(VCS_STATE_IDLE),
      mOperationId(0),
      mChallenge(0),
      mActiveGid(0),
      mCurrentEnrollFid(0),
      mSecureUserId(0),
      mAuthenticatorId(0),
      mInitialized(false) {
    ALOGI("VcsClientWrapper instance created");
}

VcsClientWrapper::~VcsClientWrapper() {
    release();
}

uint64_t VcsClientWrapper::generateRandom64() {
    uint64_t r = (((uint64_t)rand()) << 32) | ((uint64_t)rand());
    return (r != 0) ? r : 1;
}

bool VcsClientWrapper::init() {
    std::lock_guard<std::mutex> lock(mLock);
    if (mInitialized && mValidity != nullptr) {
        ALOGD("VcsClientWrapper already initialized");
        return true;
    }

    ALOGI("Initializing Validity native client interface via libvfmClient.so...");

    vcs::vcsInterface_t iface;
    iface.clientName = CLIENT_PIPE_NAME;
    iface.listener = this;

    // Tenta conectar ao daemon vcsFPService via named pipes do libvfmClient
    int ret = -1;
    for (int retry = 0; retry < 5; ++retry) {
        ret = vcs::CreateInterface(&iface, mValidity);
        if (ret == 0 && mValidity != nullptr) {
            break;
        }
        ALOGW("vcs::CreateInterface attempt %d failed (ret=%d), retrying in 500ms...", retry + 1, ret);
        usleep(500000);
    }

    if (ret != 0 || mValidity == nullptr) {
        ALOGE("Failed to create Validity interface! vcsFPService daemon may not be running.");
        mInitialized = false;
        return false;
    }

    // Configura nível de segurança padrão
    mValidity->SetSecurityLevel(VCS_SECURITY_LEVEL_REGULAR);

    // Consulta máscara inicial de dedos cadastrados
    unsigned int mask = 0;
    ret = mValidity->GetEnrolledFingerList("android", mask);
    if (ret == 0) {
        mAuthenticatorId = mask;
        ALOGI("Initial enrolled finger mask: 0x%x", mask);
    } else {
        mAuthenticatorId = 1;
        ALOGW("GetEnrolledFingerList failed during init (ret=%d), defaulting auth ID to 1", ret);
    }

    mInitialized = true;
    mState = VCS_STATE_IDLE;
    ALOGI("Validity native client interface successfully initialized");
    return true;
}

void VcsClientWrapper::release() {
    std::lock_guard<std::mutex> lock(mLock);
    if (mValidity != nullptr) {
        ALOGI("Destroying Validity interface...");
        unsigned int cancelRes = 0;
        mValidity->Cancel(cancelRes);
        vcs::DestroyInterface(mValidity);
        mValidity = nullptr;
    }
    mInitialized = false;
    mState = VCS_STATE_EXIT;
}

void VcsClientWrapper::setCallback(IVcsClientCallback* cb) {
    std::lock_guard<std::mutex> lock(mLock);
    mCallback = cb;
    ALOGD("Client callback registered");
}

uint64_t VcsClientWrapper::preEnroll() {
    std::lock_guard<std::mutex> lock(mLock);
    mChallenge = generateRandom64();
    ALOGD("preEnroll: challenge=%" PRIu64, mChallenge);
    return mChallenge;
}

int VcsClientWrapper::postEnroll() {
    std::lock_guard<std::mutex> lock(mLock);
    ALOGD("postEnroll: resetting challenge");
    mChallenge = 0;
    return 0;
}

uint32_t VcsClientWrapper::getEnrolledFingerMask() {
    if (!mInitialized || mValidity == nullptr) {
        return 0;
    }
    unsigned int mask = 0;
    int ret = mValidity->GetEnrolledFingerList("android", mask);
    if (ret == 0) {
        return mask;
    }
    ALOGW("getEnrolledFingerMask failed (ret=%d)", ret);
    return 0;
}

int VcsClientWrapper::enroll(const uint8_t* hat_bytes, size_t hat_len, uint32_t gid, uint32_t /*timeout_sec*/) {
    std::lock_guard<std::mutex> lock(mLock);
    ALOGI("enroll: gid=%u, hat_len=%zu", gid, hat_len);

    if (!mInitialized || mValidity == nullptr) {
        ALOGE("enroll: client not initialized");
        return -ENODEV;
    }

    if (hat_bytes == nullptr || hat_len < sizeof(hw_auth_token_t)) {
        ALOGE("enroll: null or invalid auth token");
        return -EINVAL;
    }

    const hw_auth_token_t* hat = reinterpret_cast<const hw_auth_token_t*>(hat_bytes);
    if (hat->version != HW_AUTH_TOKEN_VERSION) {
        ALOGE("enroll: unsupported auth token version %u", hat->version);
        return -EPROTONOSUPPORT;
    }

    if (hat->challenge == mChallenge) {
        mSecureUserId = hat->user_id;
        ALOGD("enroll: challenge verified, secure_user_id=%" PRIu64, mSecureUserId);
    } else {
        ALOGW("enroll: challenge mismatch (got %" PRIu64 ", expected %" PRIu64 ")",
              hat->challenge, mChallenge);
        return -EPERM;
    }

    mActiveGid = gid;

    // Encontra o próximo índice de dedo disponível (1 a MAX_NUM_FINGERS)
    uint32_t mask = getEnrolledFingerMask();
    uint32_t nextFid = 1;
    for (uint32_t idx = 1; idx <= MAX_NUM_FINGERS; ++idx) {
        if (!((mask >> idx) & 1)) {
            nextFid = idx;
            break;
        }
    }

    mCurrentEnrollFid = nextFid;
    mState = VCS_STATE_ENROLL;

    ALOGI("Starting enroll operation for finger index %u (enrolled mask=0x%x)...", nextFid, mask);
    vcs::vcsEnrollUser_t enrollData(nextFid, "android", VCS_ENROLL_MODE_DEFAULT);
    int ret = mValidity->Enroll(enrollData);
    if (ret != VCS_RESULT_OK) {
        ALOGE("mValidity->Enroll failed with error %d", ret);
        mState = VCS_STATE_IDLE;
        return -EIO;
    }

    return 0;
}

int VcsClientWrapper::authenticate(uint64_t operation_id, uint32_t gid) {
    std::lock_guard<std::mutex> lock(mLock);
    ALOGI("authenticate: op_id=%" PRIu64 ", gid=%u", operation_id, gid);

    if (!mInitialized || mValidity == nullptr) {
        ALOGE("authenticate: client not initialized");
        return -ENODEV;
    }

    mOperationId = operation_id;
    mActiveGid = gid;
    mState = VCS_STATE_SCAN;

    int ret = mValidity->Identify("android");
    if (ret != VCS_RESULT_OK) {
        ALOGE("mValidity->Identify failed with error %d", ret);
        mState = VCS_STATE_IDLE;
        return -EIO;
    }

    return 0;
}

int VcsClientWrapper::cancel() {
    std::lock_guard<std::mutex> lock(mLock);
    ALOGI("cancel: aborting active biometric operation");

    if (!mInitialized || mValidity == nullptr) {
        return 0;
    }

    mState = VCS_STATE_CANCELING;
    unsigned int cancelRes = 0;
    int ret = mValidity->Cancel(cancelRes);
    ALOGD("mValidity->Cancel returned %d, result=%u", ret, cancelRes);

    mState = VCS_STATE_IDLE;
    sendError(FINGERPRINT_ERROR_CANCELED, 0);
    return 0;
}

int VcsClientWrapper::remove(uint32_t gid, uint32_t fid) {
    std::lock_guard<std::mutex> lock(mLock);
    ALOGI("remove: gid=%u, fid=%u", gid, fid);

    if (!mInitialized || mValidity == nullptr) {
        return -ENODEV;
    }

    int ret = 0;
    if (fid == 0) {
        // Remover todos os dedos (índice especial 21)
        ALOGI("Removing all enrolled fingerprints (fid=0)...");
        ret = mValidity->RemoveEnrolledFinger("android", 21);
        if (ret == VCS_RESULT_OK) {
            for (uint32_t idx = 1; idx <= MAX_NUM_FINGERS; ++idx) {
                if (mCallback != nullptr) {
                    mCallback->onRemoved(idx, gid);
                }
            }
        }
    } else {
        ALOGI("Removing finger index %u...", fid);
        ret = mValidity->RemoveEnrolledFinger("android", fid);
        if (ret == VCS_RESULT_OK && mCallback != nullptr) {
            mCallback->onRemoved(fid, gid);
        }
    }

    // Atualiza ID de autenticador após remoção
    mAuthenticatorId = getEnrolledFingerMask();

    if (ret != VCS_RESULT_OK) {
        ALOGE("RemoveEnrolledFinger failed with error %d", ret);
        sendError(FINGERPRINT_ERROR_UNABLE_TO_REMOVE, 0);
        return -EIO;
    }

    return 0;
}

int VcsClientWrapper::enumerate() {
    std::lock_guard<std::mutex> lock(mLock);
    ALOGI("enumerate: listing enrolled fingerprints for gid=%u", mActiveGid);

    if (!mInitialized || mValidity == nullptr) {
        return -ENODEV;
    }

    uint32_t mask = getEnrolledFingerMask();
    ALOGD("enumerate: mask=0x%x", mask);

    // Conta dedos registrados
    int count = 0;
    for (uint32_t idx = 1; idx <= MAX_NUM_FINGERS; ++idx) {
        if ((mask >> idx) & 1) {
            count++;
        }
    }

    if (count == 0) {
        if (mCallback != nullptr) {
            mCallback->onEnumerate(0, mActiveGid, 0);
        }
        return 0;
    }

    int remaining = count;
    for (uint32_t idx = 1; idx <= MAX_NUM_FINGERS; ++idx) {
        if ((mask >> idx) & 1) {
            remaining--;
            if (mCallback != nullptr) {
                mCallback->onEnumerate(idx, mActiveGid, remaining);
            }
        }
    }

    return 0;
}

uint64_t VcsClientWrapper::getAuthenticatorId() {
    std::lock_guard<std::mutex> lock(mLock);
    uint32_t mask = getEnrolledFingerMask();
    mAuthenticatorId = (mask != 0) ? mask : 1;
    return mAuthenticatorId;
}

int VcsClientWrapper::setActiveGroup(uint32_t gid, const char* /*storePath*/) {
    std::lock_guard<std::mutex> lock(mLock);
    ALOGD("setActiveGroup: gid=%u", gid);
    mActiveGid = gid;
    return 0;
}

void VcsClientWrapper::sendAcquired(int32_t acquired_info, int32_t vendor_code) {
    if (mCallback != nullptr) {
        mCallback->onAcquired(acquired_info, vendor_code);
    }
}

void VcsClientWrapper::sendError(int32_t error, int32_t vendor_code) {
    if (mCallback != nullptr) {
        mCallback->onError(error, vendor_code);
    }
}

/**
 * Tradutor central de eventos de deslizamento nativo do Validity VFS61xx.
 */
int VcsClientWrapper::OnEvent(vcs::vcsCBEvent_t* pEvent) {
    if (pEvent == nullptr) {
        ALOGE("OnEvent: NULL event pointer!");
        return -EINVAL;
    }

    ALOGD("OnEvent: status=%u, eventId=%u, info=%u, pData=%p",
          pEvent->status, pEvent->eventId, pEvent->eventInfo, pEvent->pData);

    std::lock_guard<std::mutex> lock(mLock);

    // Se estivermos em cancelamento, descarta eventos subsequentes
    if (mState == VCS_STATE_CANCELING) {
        ALOGD("OnEvent: ignoring event %u while canceling", pEvent->eventId);
        return 0;
    }

    switch (pEvent->eventId) {
        case VCS_EVT_FINGER_DETECTED: // 11
            ALOGD("Event: Finger detected on sensor");
            sendAcquired(FINGERPRINT_ACQUIRED_GOOD, 0);
            break;

        case VCS_EVT_FINGER_REMOVED: // 20
            ALOGD("Event: Finger removed from sensor");
            break;

        case VCS_EVT_SWIPE_SPEED_UPDATE: // 42
            ALOGD("Event: Swipe speed update: %u", pEvent->eventInfo);
            if (pEvent->eventInfo == VCS_IMAGE_QUALITY_TOO_FAST) {
                sendAcquired(FINGERPRINT_ACQUIRED_TOO_FAST, 0);
            } else if (pEvent->eventInfo == VCS_IMAGE_QUALITY_TOO_SLOW) {
                sendAcquired(FINGERPRINT_ACQUIRED_TOO_SLOW, 0);
            }
            break;

        case VCS_EVT_EIV_FINGERPRINT_CAPTURED_BAD: // 45
            ALOGW("Event: Bad swipe quality captured");
            sendAcquired(FINGERPRINT_ACQUIRED_PARTIAL, 0);
            break;

        case VCS_EVT_ENROLL_CAPTURE_STATUS: { // 32
            ALOGD("Event: Enroll capture status received");
            if (pEvent->pData != nullptr) {
                const vcs::vcsEnrollCaptureStatus* status =
                    static_cast<const vcs::vcsEnrollCaptureStatus*>(pEvent->pData);

                ALOGI("Enroll status: progress=%d%%, totalSwipes=%d, badSwipes=%d, quality=0x%x",
                      status->progress, status->totalSwipes, status->badSwipes, status->imageQuality);

                // Traduz qualidade para acquiredInfo
                if (status->imageQuality & VCS_IMAGE_QUALITY_TOO_FAST) {
                    sendAcquired(FINGERPRINT_ACQUIRED_TOO_FAST, 0);
                } else if (status->imageQuality & VCS_IMAGE_QUALITY_TOO_SLOW) {
                    sendAcquired(FINGERPRINT_ACQUIRED_TOO_SLOW, 0);
                } else if (status->imageQuality & (VCS_IMAGE_QUALITY_TOO_SHORT | VCS_IMAGE_QUALITY_BAD_SWIPE)) {
                    sendAcquired(FINGERPRINT_ACQUIRED_PARTIAL, 0);
                } else if (status->imageQuality & VCS_IMAGE_QUALITY_SOMETHING_ON_THE_SENSOR) {
                    sendAcquired(FINGERPRINT_ACQUIRED_INSUFFICIENT, 0);
                } else {
                    sendAcquired(FINGERPRINT_ACQUIRED_GOOD, 0);
                }

                // Calcula amostras restantes (escala típica de 8 a 10 swipes)
                int progress = std::clamp(status->progress, 0, 100);
                int remaining = (progress >= 100) ? 0 : std::max(1, (100 - progress + 9) / 10);

                if (mCallback != nullptr && mState == VCS_STATE_ENROLL) {
                    mCallback->onEnrollResult(mCurrentEnrollFid, mActiveGid, remaining);
                }
            }
            break;
        }

        case VCS_EVT_ENROLL_SUCCESS: // 421
            ALOGI("Event: Enroll completed successfully for fid %u", mCurrentEnrollFid);
            if (mCallback != nullptr && mState == VCS_STATE_ENROLL) {
                mCallback->onEnrollResult(mCurrentEnrollFid, mActiveGid, 0);
            }
            mState = VCS_STATE_IDLE;
            mAuthenticatorId = getEnrolledFingerMask();
            break;

        case VCS_EVT_ENROLL_FAILED: // 424
            ALOGE("Event: Enroll failed");
            sendError(FINGERPRINT_ERROR_UNABLE_TO_PROCESS, 0);
            mState = VCS_STATE_IDLE;
            break;

        case VCS_EVT_IDENTIFY_SUCCESS: // 423
        case VCS_EVT_IDENTIFY_COMPLETED: { // 15
            ALOGI("Event: Identify completed, status=%u", pEvent->status);
            if (pEvent->status == VCS_RESULT_OK || pEvent->eventId == VCS_EVT_IDENTIFY_SUCCESS) {
                uint32_t fid = 0;
                if (pEvent->pData != nullptr) {
                    const vcs::vcsIdentifyResult* idResult =
                        static_cast<const vcs::vcsIdentifyResult*>(pEvent->pData);
                    fid = idResult->fingerIndex;
                    if (fid == 0 && idResult->matchedFingerIndexes[0] > 0) {
                        fid = idResult->matchedFingerIndexes[0];
                    }
                }
                if (fid == 0) {
                    fid = pEvent->eventInfo;
                }

                ALOGI("Authentication successful for finger index %u", fid);
                sendAcquired(FINGERPRINT_ACQUIRED_GOOD, 0);

                // Monta hw_auth_token_t assinado pelo TrustZone
                hw_auth_token_t hat;
                memset(&hat, 0, sizeof(hat));
                hat.version = HW_AUTH_TOKEN_VERSION;
                hat.challenge = mOperationId;
                hat.user_id = mSecureUserId;
                hat.authenticator_id = mAuthenticatorId;
                hat.authenticator_type = htobe32(HW_AUTH_FINGERPRINT);

                struct timespec ts;
                clock_gettime(CLOCK_MONOTONIC, &ts);
                hat.timestamp = htobe64((uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000);

                if (mCallback != nullptr) {
                    mCallback->onAuthenticated(fid, mActiveGid,
                        reinterpret_cast<const uint8_t*>(&hat), sizeof(hat));
                }
            } else {
                ALOGW("Authentication mismatch or rejected (status=%u)", pEvent->status);
                if (mCallback != nullptr) {
                    mCallback->onAuthenticated(0, mActiveGid, nullptr, 0);
                }
            }
            mState = VCS_STATE_IDLE;
            break;
        }

        case VCS_EVT_IDENTIFY_FAILED: // 426
            ALOGW("Event: Identify failed, no match found");
            if (mCallback != nullptr) {
                mCallback->onAuthenticated(0, mActiveGid, nullptr, 0);
            }
            mState = VCS_STATE_IDLE;
            break;

        case VCS_EVT_SENSOR_FAILED_INITIALIZATION: // 4
        case VCS_EVT_SENSOR_NON_RECOVERABLE: // 46
            ALOGE("Event: Sensor unrecoverable hardware error");
            sendError(FINGERPRINT_ERROR_HW_UNAVAILABLE, 0);
            mState = VCS_STATE_IDLE;
            break;

        case VCS_EVT_SENSOR_FINGERPRINT_CAPTURE_START: // 8
            ALOGD("Event: Sensor capture started");
            break;

        case VCS_EVT_SENSOR_FINGERPRINT_CAPTURE_COMPLETE: // 5
            ALOGD("Event: Fingerprint image capture complete");
            break;

        default:
            ALOGD("Event %u received unhandled", pEvent->eventId);
            break;
    }

    return 0;
}

} // namespace vcs
