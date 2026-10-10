# Proprietary Sensors and Validity Fingerprint for Samsung Galaxy S5 3G (k3gxx)

Este repositório contém a stack proprietária do SensorHub e do leitor de impressões digitais Validity do Samsung Galaxy S5 3G (`k3gxx`).

## Componentes

### 1. Sensores (SensorHub)
* `libsensorhub.so` / `libsensorhubservice.so`: Bibliotecas de comunicação IPC com o microcontrolador SensorHub.
* `sensorhubs.universal5422.so` / `sensors.universal5422.so`: HALs de sensores do Android (acelerômetro, giroscópio, bússola, proximidade, luminosidade, etc.).
* `sensorhubservice`: Daemon em espaço de usuário do SensorHub.
* `configs/sensorhub.conf`: Arquivo de configuração de calibração dos sensores.

### 2. Biometria (Validity Fingerprint)
* `libvcsfp.so`, `libfpasmtztransport.so`, `libvalAuth.so`, `libvfmClient.so`, `libvfmtztransport.so`: Bibliotecas proprietárias da Validity e TrustZone para captura e autenticação biométrica.
* `vcsFPService`: Daemon de serviço do leitor biométrico.
* `hal/` (`fingerprint.c`, `fp_k3gxx.h`): HAL open-source do Android para impressão digital que faz a ponte com as bibliotecas proprietárias da Validity.
