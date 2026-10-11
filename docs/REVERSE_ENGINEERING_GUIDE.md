# Guia de Engenharia Reversa: Validity VFS61xx (SM-G900H / k3gxx)

## 1. Visão Geral
Este guia consolida todas as descobertas obtidas por desmontagem, análise estática de ELF 32-bit ARM e inspeção das bibliotecas e classes Java da stack Validity VFS61xx no Samsung Galaxy S5 3G (`SM-G900H`, `universal5422`).

---

## 2. Kernel e Driver de Barramento (`vfs61xx.h`)

O leitor Validity VFS61xx comunica-se via barramento SPI (`/dev/vfsspi`). O driver do kernel (`drivers/fingerprint/vfs61xx.c`) opera com número maior `VFSSPI_MAJOR = 221`.

### 2.1 Comandos IOCTL (`VFSSPI_IOCTL_MAGIC = 'k'`)
| Comando IOCTL | Código / Tipo | Descrição |
| :--- | :--- | :--- |
| `VFSSPI_IOCTL_DEVICE_RESET` | `_IO('k', 2)` | Reset por hardware do sensor via GPIO |
| `VFSSPI_IOCTL_SET_CLK` | `_IOW('k', 3, uint)` | Ajusta a taxa de clock SPI |
| `VFSSPI_IOCTL_REGISTER_DRDY_SIGNAL` | `_IOW('k', 5, uint)` | Registra sinal assíncrono para a linha DRDY (Data Ready) |
| `VFSSPI_IOCTL_SET_DRDY_INT` | `_IOW('k', 8, uint)` | Habilita/desabilita interrupção DRDY no kernel |
| `VFSSPI_IOCTL_DEVICE_SUSPEND` | `_IO('k', 9)` | Coloca o sensor em modo de baixo consumo (power down) |
| `VFSSPI_IOCTL_POWER_ON` | `_IO('k', 13)` | Liga os reguladores de tensão LDO do sensor |
| `VFSSPI_IOCTL_POWER_OFF` | `_IO('k', 14)` | Desliga a alimentação do sensor |
| `VFSSPI_IOCTL_GET_SENSOR_ORIENT` | `_IOR('k', 18, uint)` | Obtém a orientação geométrica do sensor |
| `VFSSPI_IOCTL_SET_SENSOR_TYPE` | `_IOW('k', 20, uint)` | Define o modelo do sensor (VFS61xx) |

---

## 3. Desmontagem e Engenharia Reversa de Binários

### 3.1 `libvfmClient.so`
- **SONAME**: `libvfmClient.so`
- **Arquitetura**: ARM 32-bit ELF (EABI5)
- **Funções Exportadas Chave**:
  - `vcs::CreateInterface(const vcs::vcsInterface_t*, vcs::IValidity*&)`
  - `vcs::DestroyInterface(vcs::IValidity*)`
- **Hierarquia de Classes e Interfaces**:
  - `vcs::CAPIHandlerClient`: Classe de controle cliente.
  - Herança Múltipla:
    - Offset `+0`: `vcs::CAPIHandlerBase` / `vcs::CAPIHandler`
    - Offset `+16` (`0x10`): `vcs::IValidity` (que herda primariamente de `vcs::IFingerprint`)
    - Offset `+20` (`0x14`): `vcs::INavigation`
    - Offset `+24` (`0x18`): `vcs::IPayload`
    - Offset `+28` (`0x1c`): `vcs::IUtility`

#### Tabela Virtual `vcs::IFingerprint` (offset +16 de `CAPIHandlerClient`):
1. `Identify(const char* userId)`
2. `Cancel(unsigned int& result)`
3. `GetUserList(unsigned int& count, char** list)`
4. `GetEnrolledFingerList(const char* userId, unsigned int& mask)`
5. `SetSecurityLevel(unsigned int level)`
6. `Enroll(vcs::vcsEnrollUser_t& enrollData)`
7. `RemoveEnrolledFinger(const char* userId, unsigned int fingerIndex)`
8. `GetFingerprintImage(unsigned int mode)`
9. `SetPassword(const char* userId, vcsBlobData_s* oldPwd, vcsBlobData_s* newPwd)`
10. `VerifyPassword(const char* userId, vcsBlobData_s* pwd)`

#### Interface de Eventos `vcs::IListener`:
- Método virtual puro 0: `int OnEvent(vcs::vcsCBEvent_t* pEvent)`

---

## 4. Estruturas e Tipos Nativos (Layout ARM 32-bit)

### 4.1 `vcsCBEvent_t` (Tamanho: 16 bytes)
```cpp
struct vcsCBEvent_t {
    uint32_t status;     // Offset 0: Código de resultado VCS_RESULT_* (0 = OK)
    uint32_t eventId;    // Offset 4: Código de evento VCS_EVT_*
    uint32_t eventInfo;  // Offset 8: Informação adicional (fingerIndex, qualidade, etc.)
    void*    pData;      // Offset 12: Ponteiro para estrutura específica ou nulo
};
```

### 4.2 `vcsEnrollCaptureStatus` (Tamanho: 32 bytes)
```cpp
struct vcsEnrollCaptureStatus {
    int32_t imageQuality;          // Offset 0: Flag combinada de qualidade
    int32_t templateResult;        // Offset 4: Status do template
    int32_t totalSwipes;           // Offset 8: Total de swipes executados
    int32_t badSwipes;             // Offset 12: Total de swipes com erro
    int32_t progress;              // Offset 16: Porcentagem de progresso (0-100)
    int32_t cumulativeCoverageMap; // Offset 20: Mapa cumulativo de cobertura
    int32_t currentCoverageMap;    // Offset 24: Mapa da captura atual
    int32_t positionFeedback;      // Offset 28: Feedback de posicionamento do dedo
};
```

### 4.3 `vcsIdentifyResult` (Tamanho: 180 bytes)
```cpp
struct vcsIdentifyResult {
    char     userId[80];              // Offset 0: ID do usuário (string)
    char*    appData;                 // Offset 80: Dados de aplicação
    int32_t  fingerIndex;             // Offset 84: Dedo correspondido (1 a 10)
    int32_t  matchedFingerIndexes[20];// Offset 88: Máscara / índices combinados (80 bytes)
    int32_t  templateUpdated;         // Offset 168: Flag de atualização de template
    int32_t  reserved[2];             // Offset 172
};
```

### 4.4 `vcsEnrollUser_t` (Tamanho: 92 bytes)
```cpp
struct vcsEnrollUser_t {
    void*    vtable;         // Offset 0: Vtable (4 bytes)
    uint32_t fingerIndex;    // Offset 4: Dedo a ser cadastrado
    uint32_t mode;           // Offset 8: Modo (1 = DEFAULT, 2 = REENROLL)
    char     userId[80];     // Offset 12: Identificador do usuário ("android")
};
```

### 4.5 `vcsBlobData_s`
```cpp
struct vcsBlobData_s {
    uint32_t len;
    uint8_t* pData;
};
```

---

## 5. Tabela de Códigos de Eventos e Qualidade

### 5.1 Eventos Principais
- `VCS_EVT_SENSOR_FINGERPRINT_CAPTURE_START` = `8`
- `VCS_EVT_FINGER_DETECTED` = `11`
- `VCS_EVT_ENROLL_COMPLETED` = `13`
- `VCS_EVT_VERIFY_COMPLETED` = `14`
- `VCS_EVT_IDENTIFY_COMPLETED` = `15`
- `VCS_EVT_ENROLL_NEXT_CAPTURE_START` = `16`
- `VCS_EVT_EIV_FINGERPRINT_CAPTURED` = `17`
- `VCS_EVT_FINGER_REMOVED` = `20`
- `VCS_EVT_ENROLL_CAPTURE_STATUS` = `32`
- `VCS_EVT_SWIPE_DIRECTION` = `41`
- `VCS_EVT_SWIPE_SPEED_UPDATE` = `42`
- `VCS_EVT_EIV_FINGERPRINT_CAPTURED_BAD` = `45`
- `VCS_EVT_ENROLL_SUCCESS` = `421`
- `VCS_EVT_VERIFY_SUCCESS` = `422`
- `VCS_EVT_IDENTIFY_SUCCESS` = `423`
- `VCS_EVT_ENROLL_FAILED` = `424`
- `VCS_EVT_VERIFY_FAILED` = `425`
- `VCS_EVT_IDENTIFY_FAILED` = `426`

### 5.2 Qualidade de Deslizamento (`imageQuality`)
- `VCS_IMAGE_QUALITY_GOOD` = `0`
- `VCS_IMAGE_QUALITY_TOO_FAST` = `2` -> `FINGERPRINT_ACQUIRED_TOO_FAST`
- `VCS_IMAGE_QUALITY_TOO_SHORT` = `4` -> `FINGERPRINT_ACQUIRED_PARTIAL`
- `VCS_IMAGE_QUALITY_TOO_SLOW` = `16` -> `FINGERPRINT_ACQUIRED_TOO_SLOW`
- `VCS_IMAGE_QUALITY_SOMETHING_ON_THE_SENSOR` = `512` -> `FINGERPRINT_ACQUIRED_INSUFFICIENT`
- `VCS_IMAGE_QUALITY_BAD_SWIPE` = `8192` -> `FINGERPRINT_ACQUIRED_PARTIAL`
- `VCS_IMAGE_QUALITY_PRESSURE_TOO_LIGHT` = `0x10000` -> `FINGERPRINT_ACQUIRED_PARTIAL`
- `VCS_IMAGE_QUALITY_WET_FINGER` = `0x1000000` -> `FINGERPRINT_ACQUIRED_IMAGER_DIRTY`
