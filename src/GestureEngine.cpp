/*
 * Copyright (C) 2026 The LineageOS Project
 *
 * Native gesture recognition and uinput injection engine for
 * Validity VFS61xx swipe fingerprint sensor (Samsung Galaxy S5 / k3gxx).
 */

#define LOG_TAG "VcsGestureEngine"
#define LOG_NDEBUG 0

#include "GestureEngine.h"
#include "vcs/vcs_types.h"

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/ioctl.h>

#ifdef __ANDROID__
#include <cutils/properties.h>
#include <log/log.h>
#else
#include <stdio.h>
#define PROPERTY_VALUE_MAX 92
#define ALOGI(...) do { printf("[INFO] " __VA_ARGS__); printf("\n"); } while(0)
#define ALOGD(...) do { printf("[DEBUG] " __VA_ARGS__); printf("\n"); } while(0)
#define ALOGW(...) do { printf("[WARN] " __VA_ARGS__); printf("\n"); } while(0)
#define ALOGE(...) do { printf("[ERROR] " __VA_ARGS__); printf("\n"); } while(0)
static inline int property_get(const char*, char* val, const char* def) { strncpy(val, def, PROPERTY_VALUE_MAX - 1); return strlen(val); }
static inline bool property_get_bool(const char*, bool def) { return def; }
static inline int property_get_int32(const char*, int def) { return def; }
#endif

namespace vcs {

GestureEngine::GestureEngine()
    : mUinputFd(-1),
      mEnabled(true),
      mMode(GESTURE_MODE_HYBRID),
      mFingerPresent(false),
      mFingerDownTimeMs(0),
      mLastSwipeDownTimeMs(0),
      mHasReverseMotion(false),
      mHasSwipeMovement(false),
      mLastImageQuality(0) {
    ALOGI("GestureEngine constructed");
}

GestureEngine::~GestureEngine() {
    release();
}

bool GestureEngine::init() {
    std::lock_guard<std::mutex> lock(mLock);
    if (mUinputFd >= 0) {
        ALOGD("GestureEngine already initialized (fd=%d)", mUinputFd);
        return true;
    }

    // Lê configurações de persistência via propriedades do Android
    char modeProp[PROPERTY_VALUE_MAX] = {0};
    property_get("persist.vendor.fingerprint.gesture_mode", modeProp, "hybrid");
    if (strcmp(modeProp, "nav") == 0 || strcmp(modeProp, "navigation") == 0) {
        mMode = GESTURE_MODE_NAVIGATION;
    } else if (strcmp(modeProp, "scroll") == 0) {
        mMode = GESTURE_MODE_SCROLL;
    } else {
        mMode = GESTURE_MODE_HYBRID;
    }

    mEnabled = property_get_bool("persist.vendor.fingerprint.gestures_enabled", 1);
    ALOGI("Initializing GestureEngine: enabled=%d, mode=%d", mEnabled, mMode);

    return createUinputDevice();
}

void GestureEngine::release() {
    std::lock_guard<std::mutex> lock(mLock);
    destroyUinputDevice();
}

bool GestureEngine::createUinputDevice() {
    const char* uinputPaths[] = { "/dev/uinput", "/dev/input/uinput", nullptr };
    int fd = -1;

    for (int i = 0; uinputPaths[i] != nullptr; ++i) {
        fd = open(uinputPaths[i], O_WRONLY | O_NONBLOCK);
        if (fd >= 0) {
            ALOGI("Opened uinput interface at %s", uinputPaths[i]);
            break;
        }
    }

    if (fd < 0) {
        ALOGE("Failed to open /dev/uinput: %s (%d)", strerror(errno), errno);
        return false;
    }

    // 1. Configura tipos de eventos suportados
    if (ioctl(fd, UI_SET_EVBIT, EV_KEY) < 0 ||
        ioctl(fd, UI_SET_EVBIT, EV_REL) < 0 ||
        ioctl(fd, UI_SET_EVBIT, EV_SYN) < 0) {
        ALOGE("ioctl UI_SET_EVBIT failed: %s", strerror(errno));
        close(fd);
        return false;
    }

    // 2. Habilita teclas de navegação do sistema Android
    const uint16_t keyCodes[] = {
        KEY_SYSTEM_NAVIGATION_UP,   // 280
        KEY_SYSTEM_NAVIGATION_DOWN, // 281
        KEY_SYSTEM_NAVIGATION_LEFT, // 282
        KEY_SYSTEM_NAVIGATION_RIGHT,// 283
        KEY_PAGEUP,                 // 104
        KEY_PAGEDOWN,               // 109
        KEY_UP,                     // 103
        KEY_DOWN,                   // 108
        KEY_BACK,                   // 158
        KEY_HOMEPAGE,               // 172
    };

    for (uint16_t code : keyCodes) {
        if (ioctl(fd, UI_SET_KEYBIT, code) < 0) {
            ALOGW("Failed to register keycode %u with uinput: %s", code, strerror(errno));
        }
    }

    // 3. Habilita eixos relativos para rolagem suave (mouse wheel)
    if (ioctl(fd, UI_SET_RELBIT, REL_WHEEL) < 0) {
        ALOGW("Failed to register REL_WHEEL with uinput: %s", strerror(errno));
    }

    // 4. Configura estrutura do dispositivo virtual uinput
    struct uinput_user_dev dev;
    memset(&dev, 0, sizeof(dev));
    strncpy(dev.name, "uinput-vfs61xx-gestures", UINPUT_MAX_NAME_SIZE - 1);
    dev.id.bustype = BUS_VIRTUAL;
    dev.id.vendor  = 0x138a; // Validity Sensors Inc.
    dev.id.product = 0x0011; // VFS61xx
    dev.id.version = 1;

    ssize_t written = write(fd, &dev, sizeof(dev));
    if (written < (ssize_t)sizeof(dev)) {
        ALOGE("Failed to write uinput_user_dev: %s", strerror(errno));
        close(fd);
        return false;
    }

    if (ioctl(fd, UI_DEV_CREATE) < 0) {
        ALOGE("ioctl UI_DEV_CREATE failed: %s", strerror(errno));
        close(fd);
        return false;
    }

    mUinputFd = fd;
    ALOGI("Virtual input device 'uinput-vfs61xx-gestures' created successfully (fd=%d)", mUinputFd);
    return true;
}

void GestureEngine::destroyUinputDevice() {
    if (mUinputFd >= 0) {
        ALOGI("Destroying virtual uinput device (fd=%d)", mUinputFd);
        ioctl(mUinputFd, UI_DEV_DESTROY);
        close(mUinputFd);
        mUinputFd = -1;
    }
}

void GestureEngine::setEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(mLock);
    mEnabled = enabled;
    ALOGI("GestureEngine enabled state set to %d", enabled);
}

bool GestureEngine::isEnabled() const {
    std::lock_guard<std::mutex> lock(mLock);
    return mEnabled;
}

void GestureEngine::setGestureMode(GestureMode mode) {
    std::lock_guard<std::mutex> lock(mLock);
    mMode = mode;
    ALOGI("GestureEngine mode changed to %d", mode);
}

GestureMode GestureEngine::getGestureMode() const {
    std::lock_guard<std::mutex> lock(mLock);
    return mMode;
}

void GestureEngine::onFingerDetected(uint64_t timestampMs) {
    std::lock_guard<std::mutex> lock(mLock);
    mFingerPresent = true;
    mFingerDownTimeMs = timestampMs;
    mHasReverseMotion = false;
    mHasSwipeMovement = false;
    mLastImageQuality = 0;
    ALOGD("onFingerDetected at %" PRIu64 " ms", timestampMs);
}

void GestureEngine::onSwipeSpeedUpdate(uint32_t speedInfo) {
    std::lock_guard<std::mutex> lock(mLock);
    mHasSwipeMovement = true;
    ALOGD("onSwipeSpeedUpdate: speed=%u", speedInfo);
}

void GestureEngine::onCaptureStatus(uint32_t imageQuality, int32_t progress) {
    std::lock_guard<std::mutex> lock(mLock);
    mHasSwipeMovement = true;
    mLastImageQuality = imageQuality;

    if (imageQuality & VCS_IMAGE_QUALITY_REVERSE_MOTION) {
        mHasReverseMotion = true;
        ALOGD("onCaptureStatus: detected REVERSE_MOTION (Swipe Up)");
    }
    ALOGD("onCaptureStatus: quality=0x%x, progress=%d", imageQuality, progress);
}

void GestureEngine::onFingerRemoved(uint64_t timestampMs) {
    std::lock_guard<std::mutex> lock(mLock);
    if (!mFingerPresent) {
        return;
    }

    mFingerPresent = false;
    ALOGD("onFingerRemoved at %" PRIu64 " ms", timestampMs);
    processGesture(timestampMs);
}

void GestureEngine::setMockInjectionCallback(std::function<void(uint16_t code, int32_t val)> cb) {
    std::lock_guard<std::mutex> lock(mLock);
    mMockCallback = cb;
}

void GestureEngine::processGesture(uint64_t timestampMs) {
    if (!mEnabled || (mUinputFd < 0 && !mMockCallback)) {
        ALOGD("GestureEngine: gestures disabled or uinput not ready, skipping gesture processing");
        return;
    }

    uint64_t duration = (timestampMs > mFingerDownTimeMs) ? (timestampMs - mFingerDownTimeMs) : 0;
    ALOGI("Processing gesture: duration=%" PRIu64 " ms, movement=%d, reverse=%d, quality=0x%x",
          duration, mHasSwipeMovement, mHasReverseMotion, mLastImageQuality);

    // 1. Toque Curto / Tap (< 180ms sem deslizamento)
    if (!mHasSwipeMovement && duration > 20 && duration <= 180) {
        int tapBehavior = property_get_int32("persist.vendor.fingerprint.gesture_tap", 0);
        if (tapBehavior == 1) { // Mapeado para BACK
            ALOGI("Gesture recognized: TAP -> Emitting BACK");
            injectClick(KEY_BACK);
            return;
        } else if (tapBehavior == 2) { // Mapeado para HOME
            ALOGI("Gesture recognized: TAP -> Emitting HOME");
            injectClick(KEY_HOMEPAGE);
            return;
        }
        // Se tapBehavior == 0, toque curto fica ignorado para evitar toques acidentais
        return;
    }

    // 2. Toque Longo / Hold (> 500ms estático)
    if (!mHasSwipeMovement && duration >= 500) {
        ALOGI("Gesture recognized: LONG_PRESS -> Emitting BACK");
        injectClick(KEY_BACK);
        return;
    }

    // 3. Deslizamento Inverso (Baixo para Cima / Swipe Up)
    if (mHasReverseMotion) {
        if (mMode == GESTURE_MODE_SCROLL) {
            ALOGI("Gesture recognized: SWIPE_UP -> Scrolling page up");
            injectClick(KEY_PAGEUP);
            injectScrollWheel(1);
        } else {
            ALOGI("Gesture recognized: SWIPE_UP -> Collapsing notifications");
            injectClick(KEY_SYSTEM_NAVIGATION_UP);
        }
        return;
    }

    // 4. Deslizamento Natural (Cima para Baixo / Swipe Down)
    if (duration >= 50 && duration <= 800) {
        // Verifica se é Double Swipe Down (< 400ms do swipe anterior)
        if (mLastSwipeDownTimeMs > 0 && (timestampMs - mLastSwipeDownTimeMs) <= 400) {
            ALOGI("Gesture recognized: DOUBLE_SWIPE_DOWN -> Quick Settings / Fast Scroll");
            mLastSwipeDownTimeMs = 0;
            if (mMode == GESTURE_MODE_SCROLL) {
                injectClick(KEY_PAGEDOWN);
                injectScrollWheel(-2);
            } else {
                // Segundo evento expande o painel de Quick Settings no SystemUI
                injectClick(KEY_SYSTEM_NAVIGATION_DOWN);
            }
            return;
        }

        mLastSwipeDownTimeMs = timestampMs;

        if (mMode == GESTURE_MODE_SCROLL) {
            ALOGI("Gesture recognized: SWIPE_DOWN -> Scrolling page down");
            injectClick(KEY_PAGEDOWN);
            injectScrollWheel(-1);
        } else {
            ALOGI("Gesture recognized: SWIPE_DOWN -> Expanding notifications");
            injectClick(KEY_SYSTEM_NAVIGATION_DOWN);
        }
        return;
    }
}

bool GestureEngine::injectKey(uint16_t code, int32_t value) {
    if (mMockCallback) {
        mMockCallback(code, value);
        return true;
    }

    if (mUinputFd < 0) {
        return false;
    }

    struct input_event ev[2];
    memset(ev, 0, sizeof(ev));

    ev[0].type = EV_KEY;
    ev[0].code = code;
    ev[0].value = value;

    ev[1].type = EV_SYN;
    ev[1].code = SYN_REPORT;
    ev[1].value = 0;

    ssize_t written = write(mUinputFd, ev, sizeof(ev));
    if (written < (ssize_t)sizeof(ev)) {
        ALOGE("Failed to inject key event (code=%u, val=%d): %s", code, value, strerror(errno));
        return false;
    }

    return true;
}

bool GestureEngine::injectClick(uint16_t code) {
    if (!injectKey(code, 1)) {
        return false;
    }
    // Breve intervalo para garantir processamento pelo EventHub do Android
    usleep(15000); // 15ms
    return injectKey(code, 0);
}

bool GestureEngine::injectScrollWheel(int32_t delta) {
    if (mMockCallback) {
        mMockCallback(REL_WHEEL, delta);
        return true;
    }

    if (mUinputFd < 0) {
        return false;
    }

    struct input_event ev[2];
    memset(ev, 0, sizeof(ev));

    ev[0].type = EV_REL;
    ev[0].code = REL_WHEEL;
    ev[0].value = delta;

    ev[1].type = EV_SYN;
    ev[1].code = SYN_REPORT;
    ev[1].value = 0;

    ssize_t written = write(mUinputFd, ev, sizeof(ev));
    if (written < (ssize_t)sizeof(ev)) {
        ALOGE("Failed to inject scroll wheel event (delta=%d): %s", delta, strerror(errno));
        return false;
    }

    return true;
}

} // namespace vcs
