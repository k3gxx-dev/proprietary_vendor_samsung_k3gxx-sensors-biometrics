# Single Source of Truth (SoT) Reference Mirror

Este diretório contém cópias de referência estática dos componentes externos do ecossistema do leitor biométrico Validity VFS61xx no Samsung Galaxy S5 3G (`k3gxx`).

## Por que estes arquivos estão aqui?

Os arquivos originais residem na árvore do kernel (`kernel/samsung/k3gxx/`) e na árvore de device (`device/samsung/k3gxx/`). Eles **não podem ser movidos** de seus locais originais porque o sistema de compilação do Android e do kernel depende deles em seus caminhos específicos.

Este diretório `docs/sot/` existe para permitir que qualquer agente ou engenheiro de software reversa clone **apenas este repositório** e tenha em mãos todo o contexto de tipos, structs, enums e chamadas de sistema necessários para descompilar e analisar os binários proprietários (`vcsFPService` e `libvcsfp.so`).

---

## Estrutura do Diretório

### 1. `kernel/` (Driver de Baixo Nível)
*Origem na árvore completa:* `kernel/samsung/k3gxx/drivers/fingerprint/`
* **`vfs61xx.h`**: **Arquivo mais crítico.** Define os números de `ioctl()` do dispositivo `/dev/vfsspi`, structs de transferência SPI (`vfs_ioc_transfer`), registradores de hardware e comandos de clock/reset.
* **`vfs61xx.c`**: Driver Linux SPI. Mostra como o kernel trata a linha de interrupção (GPIO IRQ), timings de reset elétrico e sincronização de transferências.
* **`fingerprint.h`** e **`fingerprint_sysfs.c`**: Interface sysfs exposta em `/sys/class/fingerprint/fingerprint/`.

### 2. `java-api/` (A "Pedra de Roseta" dos Enums e Eventos)
*Origem na árvore completa:* `device/samsung/k3gxx/validityservice/src/com/validity/fingerprint/`
* **`VcsEvents.java`**: Tabela completa com os códigos numéricos dos eventos disparados pelo daemon (`VCS_EVT_ENROLL_SUCCESS`, `VCS_EVT_IDENTIFY_SUCCESS`, `VCS_EVT_FINGER_DETECTED`, etc.).
* **`EnrollCaptureStatus.java`**: Códigos numéricos de qualidade de captura (*dedo rápido*, *dedo lento*, *parcial*, *molhado*, *pressão insuficiente*).
* **`SensorInfo.java`**: Dimensões da matriz do sensor e parâmetros de varredura.
* **`IdentifyResult.java`**: Estrutura do resultado de autenticação biométrica.
* **`FingerprintCore.java`**: Interface JNI que mapeia as chamadas Java para os sockets nativos.
* **`ValidityService.java`**: Implementação do serviço Android que orquestra a comunicação.

### 3. `init/` (Inicialização e Permissões)
*Origem na árvore completa:* `device/samsung/k3gxx/ramdisk/`
* **`init.fingerprint.rc`**: Regras do `ueventd` que definem permissões de `/dev/vfsspi` (`0660 system system`) e declaração de inicialização do daemon `vcsFPService`.

### 4. `sepolicy/` (Políticas de Segurança do SELinux)
*Origem na árvore completa:* `device/samsung/k3gxx/sepolicy/vendor/vcs.te`
* **`vcs.te`**: Declara as permissões do daemon `vcs` para acessar `/dev/vfsspi`, sockets do TrustZone (`tee:unix_stream_socket connectto`), buffers de memória compartilhada `ion_device` e `secmem_device`.

### 5. `mobicore/` (Interface TrustZone / TEE)
*Origem na árvore completa:* `hardware/samsung_slsi/exynos5420/mobicore/include/Public/mc_linux.h`
* **`mc_linux.h`**: Comandos e structs de baixo nível para comunicação com o driver do MobiCore no kernel (`/dev/mobicore`).
