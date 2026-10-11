# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this repository is

Vendor blob and HAL tree for the SensorHub and Validity fingerprint stack on the Samsung Galaxy S5 3G (`SM-G900H`, codename `k3gxx`, Exynos 5422). It is meant to be dropped into an AOSP/LineageOS 18.1 tree at `vendor/samsung/k3gxx/sensors-biometrics`. Most files are prebuilt proprietary binaries; the only source code is `hal/fingerprint.c`.

There is no standalone build, lint, or test tooling. Nothing here compiles or runs outside the Android build system, and there is no test suite. Verification happens by building the device tree and checking on hardware. The enclosing `k3gxx/` workspace (`../CLAUDE.md`) owns the remote build runbook and the AOSP tree layout.

## Build wiring

- `Android.mk` is the entry point. It defines `k3gxx-sensor-prebuilt` and `k3gxx-sensor-exec` macros and instantiates one module per blob, then includes `hal/Android.mk` and `service/Android.mk`.
- `sensors-blobs.mk` is the product makefile fragment. It copies `configs/sensorhub.conf` to `$(TARGET_COPY_OUT_VENDOR)/etc/` and adds `fingerprint.universal5422` and `android.hardware.biometrics.fingerprint@2.1-service.universal5422` to `PRODUCT_PACKAGES`.
- All prebuilt modules set `LOCAL_CHECK_ELF_FILES := false` and `LOCAL_MULTILIB := 32`. Keep both when adding a module, since these are 32-bit ARM blobs.
- Modules with `,hw` as the third argument install under `lib/hw`, which is where the HAL loader looks for `sensors.universal5422` and `sensorhubs.universal5422`.
- `hal/Android.mk` builds `fingerprint.universal5422` into `lib/hw` linking `libvfmClient`, `liblog`, `libcutils`, `libhardware`, and `libutils`.
- `service/Android.mk` builds the HIDL 2.1 daemon `android.hardware.biometrics.fingerprint@2.1-service.universal5422`.

## Tooling and Environment Rules

- **Root Access:** If root access or `sudo` is needed on the local environment, always use `ssh root@localhost`.
- **Docker Usage:** Use Docker containers whenever necessary for codebase-specific tools (e.g. specialized compilers, disassembly tools, cross-compilation environments).
- **Idempotency:** Always create idempotent tools and scripts for repetitive tasks (e.g. syncing, building, cleaning, flashing). Safe reruns must never duplicate data, break permissions, or corrupt state.

## Architecture: two independent stacks

**Sensors (SensorHub).** `sensorhubservice` (a daemon) and `libsensorhub.so` / `libsensorhubservice.so` talk to the STM32F401 SensorHub MCU through `/dev/ssp_sensorhub`. The `sensors.universal5422.so` and `sensorhubs.universal5422.so` HALs in `proprietary/lib/hw/` sit on top of that and provide the Android sensor interface. `configs/sensorhub.conf` declares the device node, MCU model, and the MAX86900 heart-rate sensor wiring.

**Fingerprint (Validity VFS61xx).** The modern native biometrics stack operates without `ValidityService.apk` or legacy sockets:
- `include/vcs/`: C++ native header definitions (`vcs_types.h`, `CAPIHandler.h`, `vfmtztransport.h`) matching the 32-bit ARM ABI layout.
- `src/VcsClientWrapper.cpp`: C++ abstraction layer implementing `vcs::IListener` and talking directly to `libvfmClient.so` via `vcs::CreateInterface` / `vcs::IValidity`.
- `service/`: Native Android 11 HIDL 2.1 service (`BiometricsFingerprint.cpp` and `service.cpp`) implementing `android::hardware::biometrics::fingerprint::V2_1::IBiometricsFingerprint`.
- `hal/fingerprint.cpp`: Legacy HAL 2.0 interface (`fingerprint.universal5422.so`) backed by `VcsClientWrapper` for drop-in compatibility.

The closed-source side is `libvcsfp.so`, `libvfmClient.so`, `libvalAuth.so`, and TrustZone transports `libvfmtztransport.so` and `libfpasmtztransport.so` talking to MobiCore.

## Conventions specific to this repo

- `include/vcs/` is the single source of truth for the native C++ Validity types and interface headers.
- Use Android `ALOG*` macros with descriptive tags (`BiometricsFingerprintHal`, `VcsClientWrapper`, `FingerprintHal_universal5422`).
- Keep copyright headers across all source files.
- Do not commit generated build output, logs, or untracked temporary files.
- Blobs in `proprietary/` are binary artifacts; do not modify them manually.

