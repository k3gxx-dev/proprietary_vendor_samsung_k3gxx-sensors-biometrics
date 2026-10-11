# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this repository is

Vendor blob and HAL tree for the SensorHub and Validity fingerprint stack on the Samsung Galaxy S5 3G (`SM-G900H`, codename `k3gxx`, Exynos 5422). It is meant to be dropped into an AOSP/LineageOS 18.1 tree at `vendor/samsung/k3gxx/sensors-biometrics`. Most files are prebuilt proprietary binaries; the only source code is `hal/fingerprint.c`.

There is no standalone build, lint, or test tooling. Nothing here compiles or runs outside the Android build system, and there is no test suite. Verification happens by building the device tree and checking on hardware. The enclosing `k3gxx/` workspace (`../CLAUDE.md`) owns the remote build runbook and the AOSP tree layout.

## Build wiring

- `Android.mk` is the entry point. It defines `k3gxx-sensor-prebuilt` and `k3gxx-sensor-exec` macros and instantiates one module per blob, then includes `hal/Android.mk`.
- `sensors-blobs.mk` is the product makefile fragment. It copies `configs/sensorhub.conf` to `$(TARGET_COPY_OUT_VENDOR)/etc/`, so the device product tree must include it.
- All prebuilt modules set `LOCAL_CHECK_ELF_FILES := false` and `LOCAL_MULTILIB := 32`. Keep both when adding a module, since these are 32-bit ARM blobs.
- Modules with `,hw` as the third argument install under `lib/hw`, which is where the HAL loader looks for `sensors.universal5422` and `sensorhubs.universal5422`. Adding a `hw` module without that argument silently breaks HAL discovery.
- `hal/Android.mk` builds `fingerprint.universal5422` into `lib/hw` and links `liblog` and `libcutils`.

## Architecture: two independent stacks

**Sensors (SensorHub).** `sensorhubservice` (a daemon) and `libsensorhub.so` / `libsensorhubservice.so` talk to the STM32F401 SensorHub MCU through `/dev/ssp_sensorhub`. The `sensors.universal5422.so` and `sensorhubs.universal5422.so` HALs in `proprietary/lib/hw/` sit on top of that and provide the Android sensor interface. `configs/sensorhub.conf` declares the device node, MCU model, and the MAX86900 heart-rate sensor wiring.

**Fingerprint (Validity).** The open-source HAL `hal/fingerprint.c` implements the Android `fingerprint_module_t` API (version 2.0) and does not touch the sensor hardware itself. It talks to the proprietary `vcsFPService` daemon over two abstract-namespace local sockets:

- `validityservice`: the HAL is the client. It sends one-byte command codes (`CALL_INITSERVICE`, `CALL_ENROLL`, `CALL_IDENTIFY`, `CALL_REMOVE`, `CALL_CANCEL`, `CALL_GET_ENROLLED_FINGER_LIST`, `CALL_CLEANUP`) with the active group ID appended, and reads an ASCII integer reply.
- `validityservice_callback`: the HAL's listener thread, started in `fingerprint_open`, connects here to receive asynchronous events. Those events are translated into `fingerprint_msg_t` notifications (enrolling, acquired, authenticated, removed, error) for Android's FingerprintService.

The HAL holds a single `vcs_fingerprint_device_t` with a mutex that guards `notify` callbacks, and a worker state machine (`STATE_IDLE`, `STATE_ENROLL`, `STATE_SCAN`, `STATE_EXIT`). `checkinit` blocks until `initService` completes, so calls made before the daemon is reachable wait rather than fail.

The closed-source side is `libvcsfp.so`, `libvfmClient.so`, `libvalAuth.so`, and the TrustZone transports `libvfmtztransport.so` and `libfpasmtztransport.so`. Their internals aren't in this repo. Treat their symbols and protocol as opaque; the only documented contract is the socket command set above.

## Conventions specific to this repo

- `fp_k3gxx.h` is the single source of the command IDs. Any change to the socket protocol has to be made to the header and must match what `vcsFPService` expects. The protocol is not versioned, so a mismatch will not fail loudly.
- `hal/fingerprint.c` uses the Android `ALOG*` macros with `LOG_TAG "FingerprintHal_universal5422"` and `LOG_NDEBUG 1`. Verbose traces use `ALOGV`, so enable them locally when debugging.
- Keep the copyright headers in `hal/` and in `Android.mk`. They carry the Apache-2.0 license for code derived from AOSP, CyanogenMod, and Mokee.
- Do not commit generated build output, logs, or anything under the untracked `tmp/`. `.gitignore` already covers `tmp/*` (with a `.gitkeep` exception) and `.env*`.
- Blobs in `proprietary/` are binary artifacts. Replace them only with a vendor-supplied build for the same ABI and version, and record the source in the commit message.
