# Copyright (C) 2026 The LineageOS Project
#
# SensorHub configuration for Samsung Galaxy S5 3G (k3gxx)

SEN_DIR := vendor/samsung/k3gxx/sensors-biometrics

PRODUCT_COPY_FILES += \
    $(SEN_DIR)/configs/sensorhub.conf:$(TARGET_COPY_OUT_VENDOR)/etc/sensorhub.conf

PRODUCT_PACKAGES += \
    fingerprint.universal5422 \
    android.hardware.biometrics.fingerprint@2.1-service.universal5422
