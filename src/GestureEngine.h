/*
 * Copyright (C) 2026 The LineageOS Project
 *
 * Native gesture recognition and uinput injection engine for
 * Validity VFS61xx swipe fingerprint sensor (Samsung Galaxy S5 / k3gxx).
 */

#ifndef VCS_GESTURE_ENGINE_H_
#define VCS_GESTURE_ENGINE_H_

#include <stdint.h>
#include <stddef.h>
#include <mutex>
#include <functional>
#include <linux/input.h>
#include <linux/uinput.h>

#ifndef KEY_SYSTEM_NAVIGATION_UP
#define KEY_SYSTEM_NAVIGATION_UP    280
#endif
#ifndef KEY_SYSTEM_NAVIGATION_DOWN
#define KEY_SYSTEM_NAVIGATION_DOWN  281
#endif
#ifndef KEY_SYSTEM_NAVIGATION_LEFT
#define KEY_SYSTEM_NAVIGATION_LEFT  282
#endif
#ifndef KEY_SYSTEM_NAVIGATION_RIGHT
#define KEY_SYSTEM_NAVIGATION_RIGHT 283
#endif

namespace vcs {

enum GestureType {
    GESTURE_NONE = 0,
    GESTURE_SWIPE_DOWN,
    GESTURE_SWIPE_UP,
    GESTURE_DOUBLE_SWIPE_DOWN,
    GESTURE_TAP,
    GESTURE_LONG_PRESS,
};

enum GestureMode {
    GESTURE_MODE_NAVIGATION = 0, // Notificações AOSP (padrão)
    GESTURE_MODE_SCROLL,         // Rolagem de página contínua (Page Down/Up + Wheel)
    GESTURE_MODE_HYBRID,         // Notificações + Acessibilidade + Tap
};

class GestureEngine {
public:
    GestureEngine();
    virtual ~GestureEngine();

    bool init();
    void release();

    // Eventos recebidos da máquina de estados do leitor Validity
    void onFingerDetected(uint64_t timestampMs);
    void onSwipeSpeedUpdate(uint32_t speedInfo);
    void onCaptureStatus(uint32_t imageQuality, int32_t progress);
    void onFingerRemoved(uint64_t timestampMs);

    // Controle de operação
    void setEnabled(bool enabled);
    bool isEnabled() const;
    void setGestureMode(GestureMode mode);
    GestureMode getGestureMode() const;

    // Injeção de eventos via /dev/uinput
    bool injectKey(uint16_t code, int32_t value);
    bool injectClick(uint16_t code);
    bool injectScrollWheel(int32_t delta);

    // Gancho para testes unitários automatizados sem hardware real
    void setMockInjectionCallback(std::function<void(uint16_t code, int32_t val)> cb);

private:
    bool createUinputDevice();
    void destroyUinputDevice();
    void processGesture(uint64_t timestampMs);

    mutable std::mutex mLock;
    int mUinputFd;
    bool mEnabled;
    GestureMode mMode;
    std::function<void(uint16_t code, int32_t val)> mMockCallback;

    // Rastreamento temporal e cinemático do deslizamento
    bool mFingerPresent;
    uint64_t mFingerDownTimeMs;
    uint64_t mLastSwipeDownTimeMs;
    bool mHasReverseMotion;
    bool mHasSwipeMovement;
    uint32_t mLastImageQuality;
};

} // namespace vcs

#endif // VCS_GESTURE_ENGINE_H_
