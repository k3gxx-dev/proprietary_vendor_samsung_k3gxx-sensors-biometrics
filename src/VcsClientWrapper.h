/*
 * Copyright (C) 2026 The LineageOS Project
 *
 * C++ abstraction wrapper for Validity VFS61xx client interface (libvfmClient.so).
 */

#ifndef VCS_CLIENT_WRAPPER_H_
#define VCS_CLIENT_WRAPPER_H_

#include <stdint.h>
#include <stddef.h>
#include <mutex>
#include <hardware/fingerprint.h>
#include "vcs/vcs_types.h"
#include "vcs/CAPIHandler.h"

namespace vcs {

/**
 * Interface de callback para notificar a camada superior (HAL HIDL).
 */
class IVcsClientCallback {
public:
    virtual ~IVcsClientCallback() {}
    virtual void onEnrollResult(uint32_t fid, uint32_t gid, uint32_t remaining) = 0;
    virtual void onAcquired(int32_t acquired_info, int32_t vendor_code) = 0;
    virtual void onAuthenticated(uint32_t fid, uint32_t gid, const uint8_t* hat, size_t hat_len) = 0;
    virtual void onError(int32_t error, int32_t vendor_code) = 0;
    virtual void onRemoved(uint32_t fid, uint32_t gid) = 0;
    virtual void onEnumerate(uint32_t fid, uint32_t gid, uint32_t remaining) = 0;
};

enum VcsWorkerState {
    VCS_STATE_IDLE = 0,
    VCS_STATE_ENROLL,
    VCS_STATE_SCAN,
    VCS_STATE_CANCELING,
    VCS_STATE_EXIT
};

/**
 * Abstração de cliente C++ nativo que implementa vcs::IListener
 * e consome libvfmClient.so diretamente sem sockets Java.
 */
class VcsClientWrapper : public vcs::IListener {
public:
    VcsClientWrapper();
    virtual ~VcsClientWrapper();

    bool init();
    void release();
    void setCallback(IVcsClientCallback* cb);

    uint64_t preEnroll();
    int postEnroll();
    int enroll(const uint8_t* hat, size_t hat_len, uint32_t gid, uint32_t timeout_sec);
    int authenticate(uint64_t operation_id, uint32_t gid);
    int cancel();
    int remove(uint32_t gid, uint32_t fid);
    int enumerate();
    uint64_t getAuthenticatorId();
    int setActiveGroup(uint32_t gid, const char* storePath);

    // vcs::IListener
    int OnEvent(vcs::vcsCBEvent_t* pEvent) override;

    uint32_t getEnrolledFingerMask();

private:
    uint64_t generateRandom64();
    void sendAcquired(int32_t acquired_info, int32_t vendor_code = 0);
    void sendError(int32_t error, int32_t vendor_code = 0);

    std::mutex mLock;
    IVcsClientCallback* mCallback;
    vcs::IValidity* mValidity;
    VcsWorkerState mState;

    uint64_t mOperationId;
    uint64_t mChallenge;
    uint32_t mActiveGid;
    uint32_t mCurrentEnrollFid;
    uint64_t mSecureUserId;
    uint64_t mAuthenticatorId;
    bool mInitialized;
};

} // namespace vcs

#endif // VCS_CLIENT_WRAPPER_H_
