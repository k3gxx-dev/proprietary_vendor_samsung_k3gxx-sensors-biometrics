/*
 * Copyright (C) 2026 The LineageOS Project
 *
 * C header prototypes for libvfmtztransport.so reverse engineered
 * from Samsung Galaxy S5 3G (k3gxx / universal5422) TrustZone transport.
 *
 * Interacts with MobiCore Secure OS (mcOpenSession, DCI buffer, Trustlet).
 */

#ifndef VCS_VFMTZTRANSPORT_H_
#define VCS_VFMTZTRANSPORT_H_

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// Controle de Dispositivo e Sessão TrustZone / MobiCore
// ============================================================================
int vfmInitialize(void);
int vfmUninitialize(void);
int vfmProvision(void* param1, void* param2);

int vfmTZDeviceOpen(void);
int vfmTZDeviceClose(void);
int vfmTZSessionOpen(void);
int vfmTZSessionClose(void);

int vfmDeviceInitialize(void);
int vfmDeviceCalibrate(void* calData);

// ============================================================================
// Sessão de Autenticação e Captura de Imagem
// ============================================================================
int vfmAuthSessionBegin(void* sessionInfo);
int vfmAuthSessionEnd(void);

int vfmCaptureStart(void* captureParams);
int vfmCaptureReadData(void* buffer, uint32_t size);
int vfmCaptureProcessData(void* inData, void* outData);
int vfmCaptureGetStatus(uint32_t* status);
int vfmCaptureFlushImage(void);
int vfmCaptureAbort(void);

// ============================================================================
// Matrícula (Enroll) e Matching de Templates
// ============================================================================
int vfmEnrollBegin(uint32_t fingerIndex);
int vfmEnrollAddImage(void* imageData);
int vfmEnrollFinish(void* templateData);

int vfmEnrollmentPasswordSet(const char* userId, void* oldPwd, void* newPwd, uint32_t flags);
int vfmEnrollmentPasswordVerify(const char* userId, void* pwd, uint32_t flags);

int vfmMatchImageToTemplates(void* img, void* templates, uint32_t count, void* result);

// ============================================================================
// Payloads e Operações do Fabricante
// ============================================================================
int vfmPayloadBind(void* inPayload, void* outPayload);
int vfmPayloadRelease(void* payload);
int vfmVendorDefinedOperation(uint32_t opCode, void* inData, void* outData);

#ifdef __cplusplus
}
#endif

#endif // VCS_VFMTZTRANSPORT_H_
