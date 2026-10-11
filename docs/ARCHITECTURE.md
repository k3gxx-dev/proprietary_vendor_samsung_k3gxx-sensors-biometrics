# Arquitetura da Pilha Biométrica do Samsung Galaxy S5 (k3gxx)

Este documento descreve detalhadamente a arquitetura de hardware, kernel, middleware proprietário e integração com o Android para o leitor de impressão digital do **Samsung Galaxy S5 3G (`SM-G900H` / `k3gxx`, Exynos 5422)**.

---

## 1. Visão Geral das Camadas

A pilha biométrica do Galaxy S5 é única porque combina um **sensor capacitivo de deslizamento (*swipe sensor*)** lançado na era do Android 4.4 (KitKat) com as APIs modernas de biometria introduzidas a partir do Android 6.0 e presentes no Android 11.

```mermaid
flowchart TD
    subgraph AndroidFramework ["Android 11 Framework"]
        BP["BiometricPrompt / FingerprintManager"]
    end

    subgraph OpenSourceHAL ["Camada Open-Source (LineageOS)"]
        HAL["hal/fingerprint.c (fingerprint.universal5422.so)"]
        VS_APK["ValidityService.apk (Java Background Service)"]
    end

    subgraph ProprietaryVendor ["Middleware Proprietário (32-bit ELF)"]
        DAEMON["vcsFPService (Daemon em /vendor/bin)"]
        LIB_CORE["libvcsfp.so (Motor de Captura e Estado)"]
        LIB_AUTH["libvalAuth.so (Cálculo de Minúcias)"]
        LIB_TZ["libvfmtztransport.so (Transporte TEE)"]
    end

    subgraph SecureWorld ["ARM TrustZone (MobiCore T-Base)"]
        MC_DAEMON["mcDriverDaemon"]
        TRUSTLET["Trustlet de Biometria (*.tlbin)"]
    end

    subgraph KernelHW ["Kernel Linux & Hardware"]
        KDRIVER["kernel/drivers/fingerprint/vfs61xx.c (/dev/vfsspi)"]
        HW["Hardware Físico: Sensor Validity VFS61xx (Botão Home)"]
    end

    BP <==> |Binder / HIDL 2.1| HAL
    HAL <==> |UNIX Sockets: validityservice & validityservice_callback| VS_APK
    VS_APK <==> |JNI / IPC Local| DAEMON
    DAEMON --> LIB_CORE
    LIB_CORE --> LIB_AUTH
    LIB_CORE --> LIB_TZ
    LIB_TZ <==> |libMcClient.so (/dev/mobicore)| MC_DAEMON
    MC_DAEMON <==> |SMC - Secure Monitor Call| TRUSTLET
    DAEMON <==> |ioctl() em /dev/vfsspi| KDRIVER
    KDRIVER <==> |Barramento SPI & GPIO IRQ| HW
```

---

## 2. O Sensor Físico (Validity VFS61xx)

* **Tipo:** Sensor capacitivo de varredura (*swipe array*).
* **Localização:** Integrado diretamente sob o vidro do botão Home físico.
* **Barramento:** SPI mestre com linha de interrupção dedicada (GPIO IRQ) e clock configurável.
* **Mecanismo de Leitura:** 
  Ao contrário de sensores modernos de toque (*touch area sensors*) que capturam uma imagem bidimensional de 200x200 pixels instantaneamente, o VFS61xx possui apenas uma matriz linear muito estreita (uma faixa de poucos pixels de altura).
  À medida que o usuário desliza o dedo de cima para baixo sobre o botão, o sensor transmite "fatias" (*slices*) sucessivas de dados brutos via SPI em alta velocidade. O software deve juntar (*stitch*) essas fatias em um quadro coerente.

---

## 3. Fluxo de Execução de uma Leitura Biométrica

### Passo a Passo:
1. **Ativação:**  
   O usuário acende a tela ou um app bancário exibe o `BiometricPrompt`.  
   O framework chama `authenticate()` na HAL [`hal/fingerprint.c`](file:///zRAID/builds/k3gxx-lineage-18.1/vendor/samsung/k3gxx/sensors-biometrics/hal/fingerprint.c).
2. **Disparo do Comando:**  
   A HAL envia o opcode `CALL_IDENTIFY` através do socket `/dev/socket/validityservice` para o `ValidityService.apk`.
3. **Preparação do Sensor:**  
   O `vcsFPService` abre `/dev/vfsspi`, emite um `ioctl` de ativação de clock (`VFS_IOCTL_SET_CLK`), reseta o hardware e aguarda a interrupção de contato elétrico no pino GPIO (quando a pele do dedo toca no metal do botão Home).
4. **Captura do Deslizamento:**  
   Quando o dedo começa a passar, o chip dispara interrupções sucessivas. O `vcsFPService` lê os buffers de imagem crua via SPI (`VFS_IOCTL_READ`).
5. **Comunicação com o TrustZone:**  
   O `libvfmtztransport.so` aloca um buffer compartilhado (DCI Buffer) via ION/secmem e chama `mcNotify()` no MobiCore.
6. **Autenticação Segura:**  
   O trustlet seguro dentro do ARM TrustZone extrai os pontos característicos (minúcias) da imagem, compara com os templates criptografados salvos no dispositivo e responde com um código de sucesso ou falha.
7. **Callback para o Android:**  
   O resultado é despachado de volta pelo socket assíncrono `validityservice_callback`, recebido pela thread ouvinte de `fingerprint.c` e notificado ao sistema operacional através de `notify(FINGERPRINT_AUTHENTICATED)`.

---

## 4. O Protocolo de Sockets entre HAL e Middleware

Definido em [`hal/fp_k3gxx.h`](file:///zRAID/builds/k3gxx-lineage-18.1/vendor/samsung/k3gxx/sensors-biometrics/hal/fp_k3gxx.h):

| Comando (Byte Hex) | Nome Simbólico | Descrição |
| :--- | :--- | :--- |
| `0x01` | `CALL_INITSERVICE` | Inicializa a conexão com o daemon de biometria |
| `0x02` | `CALL_ENROLL` | Inicia a sessão de cadastro de uma nova digital |
| `0x03` | `CALL_IDENTIFY` | Coloca o sensor em modo de escuta para autenticação |
| `0x04` | `CALL_REMOVE` | Apaga a digital do banco de dados seguro |
| `0x05` | `CALL_CANCEL` | Aborta a operação atual (ex: tela desligou) |
| `0x06` | `CALL_GET_ENROLLED_FINGER_LIST` | Retorna máscara de bits com os dedos cadastrados |
| `0x07` | `CALL_CLEANUP` | Encerra a sessão e desliga a alimentação do sensor |

---

## 5. Gargalos Identificados na Implementação Atual

* **Duplo Intermediário IPC:** Ter `fingerprint.c` conversando com Java (`ValidityService`) e este conversando com C++ (`vcsFPService`) adiciona até 300ms de latência desnecessária no desbloqueio.
* **Problemas de Sincronismo do Swipe:** Se o usuário deslizar o dedo com muita pressa antes do socket responder `CALL_IDENTIFY`, a leitura é ignorada silenciosamente.
* **Dependência do TEE:** Como a chave de autenticação é manipulada pelo TrustZone, qualquer nova HAL deve replicar exatamente a interface do cliente MobiCore (`mc_linux.h`), sem tentar implementar o matching biométrico fora do ambiente seguro.
