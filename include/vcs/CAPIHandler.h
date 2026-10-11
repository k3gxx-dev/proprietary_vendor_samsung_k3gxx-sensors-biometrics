/*
 * Copyright (C) 2026 The LineageOS Project
 *
 * C++ interface definitions for libvfmClient.so reverse engineered
 * from Samsung Galaxy S5 3G (k3gxx / universal5422) proprietary blobs.
 */

#ifndef VCS_CAPI_HANDLER_H_
#define VCS_CAPI_HANDLER_H_

#include "vcs_types.h"

namespace vcs {

/**
 * Interface de callback assíncrona recebida pelo CAPIHandlerClient.
 * Emite os eventos de deslizamento, status de captura e resultado de biometria.
 */
class IListener {
public:
    virtual int OnEvent(vcsCBEvent_t* pEvent) = 0;
    virtual ~IListener() {}
};

/**
 * Interface primária de operações biométricas de impressão digital.
 */
class IFingerprint {
public:
    virtual int Identify(const char* userId) = 0;
    virtual int Cancel(unsigned int& result) = 0;
    virtual int GetUserList(unsigned int& count, char** list) = 0;
    virtual int GetEnrolledFingerList(const char* userId, unsigned int& mask) = 0;
    virtual int SetSecurityLevel(unsigned int level) = 0;
    virtual int Enroll(vcsEnrollUser_t& enrollData) = 0;
    virtual int RemoveEnrolledFinger(const char* userId, unsigned int fingerIndex) = 0;
    virtual int GetFingerprintImage(unsigned int mode) = 0;
    virtual int SetPassword(const char* userId, vcsBlobData_s* oldPwd, vcsBlobData_s* newPwd) = 0;
    virtual int VerifyPassword(const char* userId, vcsBlobData_s* pwd) = 0;
    virtual ~IFingerprint() {}
};

/**
 * Interface de controle de navegação pelo sensor.
 */
class INavigation {
public:
    virtual int StartNav(vcsNavStart_t* navStart) = 0;
    virtual int StopNav(vcsNavStop_t* navStop) = 0;
    virtual int SetNavConfig(unsigned int mode, vcsNavConfig_t* config) = 0;
    virtual int GetNavConfig(unsigned int mode, vcsNavConfig_t* config) = 0;
    virtual ~INavigation() {}
};

/**
 * Interface para proteção criptográfica e vinculação de payloads.
 */
class IPayload {
public:
    virtual int Protect(unsigned int alg, vcsBlobData_s* inData, vcsBlobData_s* outData) = 0;
    virtual int Unprotect(vcsBlobData_s* inData, vcsBlobData_s* outData) = 0;
    virtual ~IPayload() {}
};

/**
 * Interface de utilitários, notificações e consulta de versão.
 */
class IUtility {
public:
    virtual int Request(vcsUtilityInput_t* input) = 0;
    virtual int RequestSync(vcsUtilityInput_t* input, vcsUtilityInput_t* output) = 0;
    virtual int Notify(vcsUtilityInput_t* input) = 0;
    virtual int GetVersion(unsigned int type, vcsVersion_s& version) = 0;
    virtual int BufferFree(void* ptr) = 0;
    virtual ~IUtility() {}
};

/**
 * Interface agregada principal instanciada por vcs::CreateInterface.
 */
class IValidity : public IFingerprint,
                  public INavigation,
                  public IPayload,
                  public IUtility {
public:
    virtual ~IValidity() {}
};

/**
 * Parâmetro de configuração do cliente para vcs::CreateInterface.
 */
struct vcsInterface_t {
    const char* clientName; // Nome do cliente (e.g. "validity" ou canal de pipe)
    IListener*  listener;   // Instância de escuta de eventos nativa
};

/**
 * Funções exportadas por libvfmClient.so
 */
extern "C" {
    int CreateInterface(const vcsInterface_t* pInterfaceInfo, IValidity*& pValidity);
    int DestroyInterface(IValidity* pValidity);
}

} // namespace vcs

#endif // VCS_CAPI_HANDLER_H_
