/*
 * Copyright (C) 2026 The LineageOS Project
 *
 * Automated unit tests for Validity VFS61xx GestureEngine.
 */

#include "src/GestureEngine.h"
#include "include/vcs/vcs_types.h"
#include <iostream>
#include <vector>
#include <cassert>

using namespace vcs;

struct RecordedEvent {
    uint16_t code;
    int32_t val;
};

int main() {
    std::cout << "=== Running GestureEngine Automated Tests ===" << std::endl;

    GestureEngine engine;
    std::vector<RecordedEvent> events;

    engine.setMockInjectionCallback([&events](uint16_t code, int32_t val) {
        events.push_back({code, val});
    });

    // ------------------------------------------------------------------------
    // Test 1: Single Swipe Down (Natural Motion) -> KEY_SYSTEM_NAVIGATION_DOWN
    // ------------------------------------------------------------------------
    std::cout << "Test 1: Single Swipe Down... ";
    events.clear();
    engine.onFingerDetected(1000);
    engine.onSwipeSpeedUpdate(100);
    engine.onCaptureStatus(VCS_IMAGE_QUALITY_GOOD, 10);
    engine.onFingerRemoved(1250); // 250ms duration

    assert(events.size() == 2);
    assert(events[0].code == KEY_SYSTEM_NAVIGATION_DOWN && events[0].val == 1);
    assert(events[1].code == KEY_SYSTEM_NAVIGATION_DOWN && events[1].val == 0);
    std::cout << "PASSED (Emitted KEY_SYSTEM_NAVIGATION_DOWN)" << std::endl;

    // ------------------------------------------------------------------------
    // Test 2: Double Swipe Down (< 400ms interval) -> Quick Settings
    // ------------------------------------------------------------------------
    std::cout << "Test 2: Double Swipe Down... ";
    events.clear();
    engine.onFingerDetected(1450); // 200ms after previous removed
    engine.onSwipeSpeedUpdate(100);
    engine.onCaptureStatus(VCS_IMAGE_QUALITY_GOOD, 20);
    engine.onFingerRemoved(1600); // 150ms duration

    assert(events.size() == 2);
    assert(events[0].code == KEY_SYSTEM_NAVIGATION_DOWN && events[0].val == 1);
    assert(events[1].code == KEY_SYSTEM_NAVIGATION_DOWN && events[1].val == 0);
    std::cout << "PASSED (Emitted second KEY_SYSTEM_NAVIGATION_DOWN for Quick Settings)" << std::endl;

    // ------------------------------------------------------------------------
    // Test 3: Swipe Up (Reverse Motion) -> KEY_SYSTEM_NAVIGATION_UP
    // ------------------------------------------------------------------------
    std::cout << "Test 3: Swipe Up (Reverse Motion)... ";
    events.clear();
    engine.onFingerDetected(2500);
    engine.onSwipeSpeedUpdate(100);
    engine.onCaptureStatus(VCS_IMAGE_QUALITY_REVERSE_MOTION, 10);
    engine.onFingerRemoved(2750); // 250ms duration

    assert(events.size() == 2);
    assert(events[0].code == KEY_SYSTEM_NAVIGATION_UP && events[0].val == 1);
    assert(events[1].code == KEY_SYSTEM_NAVIGATION_UP && events[1].val == 0);
    std::cout << "PASSED (Emitted KEY_SYSTEM_NAVIGATION_UP)" << std::endl;

    // ------------------------------------------------------------------------
    // Test 4: Scroll Mode -> KEY_PAGEDOWN + REL_WHEEL (-1)
    // ------------------------------------------------------------------------
    std::cout << "Test 4: Scroll Mode (Swipe Down)... ";
    engine.setGestureMode(GESTURE_MODE_SCROLL);
    events.clear();
    engine.onFingerDetected(4000);
    engine.onSwipeSpeedUpdate(100);
    engine.onCaptureStatus(VCS_IMAGE_QUALITY_GOOD, 10);
    engine.onFingerRemoved(4250);

    assert(events.size() == 3);
    assert(events[0].code == KEY_PAGEDOWN && events[0].val == 1);
    assert(events[1].code == KEY_PAGEDOWN && events[1].val == 0);
    assert(events[2].code == REL_WHEEL && events[2].val == -1);
    std::cout << "PASSED (Emitted KEY_PAGEDOWN and REL_WHEEL -1)" << std::endl;

    // ------------------------------------------------------------------------
    // Test 5: Scroll Mode (Swipe Up) -> KEY_PAGEUP + REL_WHEEL (+1)
    // ------------------------------------------------------------------------
    std::cout << "Test 5: Scroll Mode (Swipe Up)... ";
    events.clear();
    engine.onFingerDetected(5000);
    engine.onSwipeSpeedUpdate(100);
    engine.onCaptureStatus(VCS_IMAGE_QUALITY_REVERSE_MOTION, 10);
    engine.onFingerRemoved(5250);

    assert(events.size() == 3);
    assert(events[0].code == KEY_PAGEUP && events[0].val == 1);
    assert(events[1].code == KEY_PAGEUP && events[1].val == 0);
    assert(events[2].code == REL_WHEEL && events[2].val == 1);
    std::cout << "PASSED (Emitted KEY_PAGEUP and REL_WHEEL +1)" << std::endl;

    // ------------------------------------------------------------------------
    // Test 6: Gestures Disabled -> No events emitted
    // ------------------------------------------------------------------------
    std::cout << "Test 6: Suppression when Disabled... ";
    engine.setEnabled(false);
    events.clear();
    engine.onFingerDetected(6000);
    engine.onSwipeSpeedUpdate(100);
    engine.onCaptureStatus(VCS_IMAGE_QUALITY_GOOD, 10);
    engine.onFingerRemoved(6250);

    assert(events.empty());
    std::cout << "PASSED (No events emitted when disabled)" << std::endl;

    std::cout << "\n>>> ALL 6 GESTURE ENGINE UNIT TESTS PASSED SUCCESSFULLY! <<<\n" << std::endl;
    return 0;
}
