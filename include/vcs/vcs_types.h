/*
 * Copyright (C) 2026 The LineageOS Project
 *
 * Native C++ type definitions reconstructed from proprietary Validity
 * VFS61xx libraries (libvfmClient.so, libvcsfp.so, libvfmtztransport.so).
 *
 * Strictly adheres to 32-bit ARM ABI layout and structure alignments.
 */

#ifndef VCS_TYPES_H_
#define VCS_TYPES_H_

#include <stdint.h>
#include <stddef.h>
#include <string.h>

#pragma pack(push, 4)

namespace vcs {

// ============================================================================
// Enums: Resultado e Códigos de Erro (VCS_RESULT_*)
// ============================================================================
enum VcsResult {
    VCS_RESULT_FAILED                         = -1,
    VCS_RESULT_OK                             = 0,
    VCS_RESULT_CANCEL                         = 1,
    VCS_RESULT_OUT_OF_MEMORY                  = 2,
    VCS_RESULT_NO_MORE_ENTRIES                = 3,
    VCS_RESULT_BAD_PARAM                      = 17,
    VCS_RESULT_SENSOR_NOT_FOUND               = 19,
    VCS_RESULT_SENSOR_OPEN_FAILED             = 22,
    VCS_RESULT_TIMEOUT                        = 34,
    VCS_RESULT_NULL_POINTER                   = 40,
    VCS_RESULT_SENSOR_BUSY                    = 46,
    VCS_RESULT_ERROR                          = 54,
    VCS_RESULT_TEMPLATE_FILE_CREATION_FAILED  = 64,
    VCS_RESULT_USER_DOESNT_EXIST              = 65,
    VCS_RESULT_TEMPLATE_DOESNT_EXIST          = 66,
    VCS_RESULT_INVALID_USER_HANDLE            = 70,
    VCS_RESULT_TEMPLATE_CORRUPTED             = 87,
    VCS_RESULT_USER_FINGER_ALREADY_ENROLLED   = 88,
    VCS_RESULT_USER_DATA_CORRUPTED            = 91,
    VCS_RESULT_MATCHER_OPEN_FAILED            = 300,
    VCS_RESULT_MATCHER_CLOSE_FAILED           = 301,
    VCS_RESULT_MATCHER_MATCH_FAILED           = 302,
    VCS_RESULT_MATCHER_EXTRACT_FAILED         = 303,
    VCS_RESULT_MATCHER_ENROLL_FAILED          = 304,
    VCS_RESULT_MATCHER_VERIFY_FAILED          = 305,
    VCS_RESULT_MATCHER_ADD_IMAGE_FAILED       = 306,
    VCS_RESULT_MATCHER_MATCH_IMAGE_FAILED     = 307,
    VCS_RESULT_MATCHER_CHECK_QUALITY_FAILED   = 308,
    VCS_RESULT_SERVICE_NOT_RUNNING            = 321,
    VCS_RESULT_SENSOR_NOT_READY_FOR_USE       = 358,
    VCS_RESULT_SENSOR_IS_REMOVED              = 360,
    VCS_RESULT_OPERATION_CANCELED             = 361,
    VCS_RESULT_USER_IDENTIFICATION_FAILED     = 362,
    VCS_RESULT_USER_VERIFICATION_FAILED       = 363,
    VCS_RESULT_MATCH_FAILED                   = 367,
    VCS_RESULT_SERVICE_STOPPED                = 500,
    VCS_RESULT_GET_PRINT_STOPPED              = 501,
    VCS_RESULT_OPERATION_DENIED               = 510,
    VCS_RESULT_PIPE_ERROR                     = 511,
    VCS_RESULT_SENSOR_KEYS_NOT_READY          = 512,
    VCS_RESULT_BAD_DEVICE                     = 513,
    VCS_RESULT_TOO_MANY_BAD_SWIPES            = 515,
    VCS_RESULT_BAD_QUALITY_IMAGE              = 601,
    VCS_RESULT_KEYDATA_NOT_FOUND              = 777,
    VCS_RESULT_DATA_STORE_FAILED              = 1001,
    VCS_RESULT_DATA_RETRIEVE_FAILED           = 1002,
    VCS_RESULT_DATA_REMOVE_FAILED             = 1003,
    VCS_RESULT_ALREADY_INPROGRESS             = 1004,
    VCS_RESULT_NOT_IMPLEMENTED                = 1005,
    VCS_RESULT_INVALID_ARGUMENT               = 1006,
};

// ============================================================================
// Enums: Identificadores de Eventos (VCS_EVT_*)
// ============================================================================
enum VcsEventId {
    VCS_EVT_SENSOR_REMOVED                           = 1,
    VCS_EVT_SENSOR_DETECTED                          = 2,
    VCS_EVT_SENSOR_READY_FOR_USE                     = 3,
    VCS_EVT_SENSOR_FAILED_INITIALIZATION             = 4,
    VCS_EVT_SENSOR_FINGERPRINT_CAPTURE_COMPLETE      = 5,
    VCS_EVT_SENSOR_RAW_FINGERPRINT_CAPTURE_COMPLETE  = 6,
    VCS_EVT_SENSOR_FINGERPRINT_CAPTURE_FAILED        = 7,
    VCS_EVT_SENSOR_FINGERPRINT_CAPTURE_START         = 8,
    VCS_EVT_ALL_SENSORS_INITIALIZED                  = 9,
    VCS_EVT_SENSOR_FINGERPRINT_FAILED_SWIPE_RETRY    = 10,
    VCS_EVT_FINGER_DETECTED                          = 11,
    VCS_EVT_ENROLL_COMPLETED                         = 13,
    VCS_EVT_VERIFY_COMPLETED                         = 14,
    VCS_EVT_IDENTIFY_COMPLETED                       = 15,
    VCS_EVT_ENROLL_NEXT_CAPTURE_START                = 16,
    VCS_EVT_EIV_FINGERPRINT_CAPTURED                 = 17,
    VCS_EVT_FINGER_REMOVED                           = 20,
    VCS_EVT_ENROLL_CAPTURE_STATUS                    = 32,
    VCS_EVT_FINGER_SETTLED                           = 33,
    VCS_EVT_SET_IR_FLAGS                             = 37,
    VCS_EVT_SWIPE_DIRECTION                          = 41,
    VCS_EVT_SWIPE_SPEED_UPDATE                       = 42,
    VCS_EVT_EIV_FINGERPRINT_CAPTURED_BAD             = 45,
    VCS_EVT_SENSOR_NON_RECOVERABLE                   = 46,
    VCS_EVT_ENVIRONMENT_NOISE_DETECTED               = 47,
    VCS_EVT_EIV_FINGERPRINT_CAPTURE_REDUNDANT        = 49,
    VCS_EVT_EIV_FINGERPRINT_CAPTURE_EARLY            = 50,
    VCS_EVT_ENROLL_SUCCESS                           = 421,
    VCS_EVT_VERIFY_SUCCESS                           = 422,
    VCS_EVT_IDENTIFY_SUCCESS                         = 423,
    VCS_EVT_ENROLL_FAILED                            = 424,
    VCS_EVT_VERIFY_FAILED                            = 425,
    VCS_EVT_IDENTIFY_FAILED                          = 426,
    VCS_EVT_GESTURE                                  = 427,
    VCS_EVT_SENSOR_INFO                              = 428,
};

// ============================================================================
// Enums: Qualidade da Imagem e Deslizamento (VCS_IMAGE_QUALITY_*)
// ============================================================================
enum VcsImageQuality {
    VCS_IMAGE_QUALITY_GOOD                           = 0,
    VCS_IMAGE_QUALITY_STICTION                       = 1,
    VCS_IMAGE_QUALITY_TOO_FAST                       = 2,
    VCS_IMAGE_QUALITY_TOO_SHORT                      = 4,
    VCS_IMAGE_QUALITY_REVERSE_MOTION                 = 8,
    VCS_IMAGE_QUALITY_TOO_SLOW                       = 16,
    VCS_IMAGE_QUALITY_SOMETHING_ON_THE_SENSOR        = 512,
    VCS_IMAGE_QUALITY_ONE_HAND_SWIPE                 = 1024,
    VCS_IMAGE_QUALITY_FINGER_OFFSET                  = 4096,
    VCS_IMAGE_QUALITY_BAD_SWIPE                      = 8192,
    VCS_IMAGE_QUALITY_SKEW_TOO_LARGE                 = 32768,
    VCS_IMAGE_QUALITY_PRESSURE_TOO_LIGHT             = 0x10000,
    VCS_IMAGE_QUALITY_FINGER_OFFSET_TOO_FAR_LEFT     = 0x20000,
    VCS_IMAGE_QUALITY_FINGER_OFFSET_TOO_FAR_RIGHT    = 0x40000,
    VCS_IMAGE_QUALITY_PRESSURE_TOO_HARD              = 0x80000,
    VCS_IMAGE_QUALITY_ASP_DATA_INVALID               = 0x100000,
    VCS_IMAGE_QUALITY_NOT_A_FINGER_SWIPE             = 0x200000,
    VCS_IMAGE_QUALITY_BASELINE_DATA_INVALID          = 0x400000,
    VCS_IMAGE_QUALITY_WET_FINGER                     = 0x1000000,
    VCS_IMAGE_QUALITY_FINGER_TOO_THIN                = 0x2000000,
    VCS_IMAGE_QUALITY_PARTIAL_TOUCH                  = 0x8000000,
    VCS_IMAGE_QUALITY_EMPTY_TOUCH                    = 0x10000000,
    VCS_IMAGE_QUALITY_GESTURE                        = 0x20000000,
    VCS_IMAGE_QUALITY_PROCESS_FAILED                 = 0x40000000,
    VCS_IMAGE_QUALITY_PROCESS_FAILED_FATAL           = 0x80000000,
};

// ============================================================================
// Enums: Modos de Matrícula e Segurança
// ============================================================================
enum VcsEnrollMode {
    VCS_ENROLL_MODE_DEFAULT   = 1,
    VCS_ENROLL_MODE_REENROLL  = 2,
    VCS_ENROLL_MODE_SECONDARY = 3,
};

enum VcsSecurityLevel {
    VCS_SECURITY_LEVEL_LOW       = 0,
    VCS_SECURITY_LEVEL_REGULAR   = 1,
    VCS_SECURITY_LEVEL_HIGH      = 2,
    VCS_SECURITY_LEVEL_VERY_HIGH = 3,
};

enum VcsSwipeDirection {
    VCS_SWIPE_DIRECTION_UP    = 0,
    VCS_SWIPE_DIRECTION_DOWN  = 1,
    VCS_SWIPE_DIRECTION_LEFT  = 3,
    VCS_SWIPE_DIRECTION_RIGHT = 4,
    VCS_SWIPE_DIRECTION_REST  = 5,
};

// ============================================================================
// Estruturas de Dados Nativas (layout binário ARM 32-bit)
// ============================================================================

/**
 * Evento bruto entregue à interface vcs::IListener::OnEvent.
 * Tamanho total: 16 bytes.
 */
struct vcsCBEvent_t {
    uint32_t status;     // Código de resultado VCS_RESULT_* (0 = OK)
    uint32_t eventId;    // Código do evento VCS_EVT_*
    uint32_t eventInfo;  // Informações adicionais (fid, imageQuality, etc.)
    void*    pData;      // Ponteiro para payload adicional (vcsEnrollCaptureStatus, etc.)
};

/**
 * Status detalhado da captura de matrícula (deslizamento).
 * Tamanho total: 32 bytes.
 */
struct vcsEnrollCaptureStatus {
    int32_t imageQuality;          // Qualidade da captura (VCS_IMAGE_QUALITY_*)
    int32_t templateResult;        // Status do matcher / template
    int32_t totalSwipes;           // Número total de deslizamentos efetuados
    int32_t badSwipes;             // Deslizamentos com falha
    int32_t progress;              // Progresso (0 a 100)
    int32_t cumulativeCoverageMap; // Mapa de cobertura acumulado
    int32_t currentCoverageMap;    // Mapa de cobertura da última captura
    int32_t positionFeedback;      // Feedback de posicionamento do dedo
};

/**
 * Resultado de identificação / autenticação.
 * Tamanho total: 180 bytes.
 */
struct vcsIdentifyResult {
    char     userId[80];              // Identificador do usuário ("android")
    char*    appData;                 // Metadados da aplicação (ou NULL)
    int32_t  fingerIndex;             // Dedo correspondido (1 a 10)
    int32_t  matchedFingerIndexes[20];// Índices correspondidos no TrustZone
    int32_t  templateUpdated;         // Se o template foi atualizado adaptativamente
    int32_t  reserved[2];
};

/**
 * Estrutura para passagem de dados binários / blobs criptográficos.
 */
struct vcsBlobData_s {
    uint32_t len;
    uint8_t* pData;
};

/**
 * Versão do software / firmware.
 * Tamanho total: 16 bytes.
 */
struct vcsVersion_s {
    uint32_t major;
    uint32_t minor;
    uint32_t build;
    uint32_t revision;
};

/**
 * Parâmetro para chamada de matrícula Enroll.
 * Tamanho total: 92 bytes.
 */
struct vcsEnrollUser_t {
    virtual ~vcsEnrollUser_t() {}

    uint32_t fingerIndex;   // Índice do dedo (1 a 10)
    uint32_t mode;          // Modo de matrícula (VCS_ENROLL_MODE_*)
    char     userId[80];    // ID do usuário ("android\0")

    vcsEnrollUser_t(uint32_t index = 1, const char* user = "android", uint32_t m = VCS_ENROLL_MODE_DEFAULT)
        : fingerIndex(index), mode(m) {
        memset(userId, 0, sizeof(userId));
        if (user) {
            strncpy(userId, user, sizeof(userId) - 1);
        }
    }
};

// Estruturas auxiliares de navegação e utilitários
struct vcsNavStart_t {
    uint32_t mode;
};

struct vcsNavStop_t {
    uint32_t mode;
};

struct vcsNavConfig_t {
    uint32_t config[4];
};

struct vcsUtilityInput_t {
    virtual ~vcsUtilityInput_t() {}
    uint32_t command;
    void*    pData;
};

struct vcsUtilityInArgs_t;
struct vcsFPInputPacket_t;
struct vcsIPCOutput_t;

} // namespace vcs

#pragma pack(pop)

#endif // VCS_TYPES_H_
