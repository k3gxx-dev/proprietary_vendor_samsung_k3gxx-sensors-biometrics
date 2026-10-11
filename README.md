# Proprietary Sensors and Validity Fingerprint for Samsung Galaxy S5 3G (k3gxx)

Este repositório contém a stack proprietária do SensorHub e do leitor de impressões digitais Validity do Samsung Galaxy S5 3G (`k3gxx`).

## Componentes

### 1. Sensores (SensorHub)
* `libsensorhub.so` / `libsensorhubservice.so`: Bibliotecas de comunicação IPC com o microcontrolador SensorHub.
* `sensorhubs.universal5422.so` / `sensors.universal5422.so`: HALs de sensores do Android (acelerômetro, giroscópio, bússola, proximidade, luminosidade, etc.).
* `sensorhubservice`: Daemon em espaço de usuário do SensorHub.
* `configs/sensorhub.conf`: Arquivo de configuração de calibração dos sensores.

### 2. Biometria (Validity Fingerprint)
* `libvcsfp.so`, `libfpasmtztransport.so`, `libvalAuth.so`, `libvfmClient.so`, `libvfmtztransport.so`: Bibliotecas proprietárias da Validity e TrustZone para captura e autenticação biométrica (MobiCore).
* `vcsFPService`: Daemon de serviço do leitor biométrico proprietário.
* `include/vcs/`: Cabeçalhos C++ nativos reconstruídos por engenharia reversa (`vcs_types.h`, `CAPIHandler.h`, `vfmtztransport.h`).
* `src/VcsClientWrapper.cpp`: Abstração C++ cliente conectando-se diretamente ao `libvfmClient.so` via IPC nativo (/dev/validity).
* `service/`: Daemon Android 11 HIDL 2.1 (`android.hardware.biometrics.fingerprint@2.1-service.universal5422`).
* `hal/` (`fingerprint.cpp`): HAL C++ legada (fingerprint.universal5422.so) compatível com AOSP 2.0.

## Diretrizes e Ferramentas

- **Acesso root:** Se for necessário privilégio root ou `sudo` no ambiente local, utilize `ssh root@localhost`.
- **Uso de Docker:** Utilize Docker sempre que necessário para ferramentas específicas desta codebase (ex.: cross-compilers, descompiladores ou utilitários isolados).
- **Idempotência:** Crie sempre ferramentas e scripts idempotentes para tarefas repetitivas (sincronização, builds, testes). Re-execuções seguras não devem duplicar dados, corromper configurações ou vazar credenciais.

