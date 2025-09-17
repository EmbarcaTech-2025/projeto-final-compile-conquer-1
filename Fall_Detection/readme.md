# Idoso Seguro: Módulo de Detecção de Quedas (Pico W)

Este diretório contém o firmware do dispositivo vestível de detecção de quedas do projeto Idoso Seguro. O sistema roda em um Raspberry Pi Pico W e integra sensores inerciais (MPU6050), GPS (ATGM336H), botões/atuadores e conectividade Wi‑Fi para notificar cuidadores via Telegram. A aplicação usa FreeRTOS, watchdog e um servidor web embutido para configuração inicial.

---

## 🎯 Visão Geral

O dispositivo monitora continuamente aceleração e giroscópio e executa inferência de Machine Learning (Edge Impulse / TensorFlow Lite) para detectar quedas com baixa latência e boa robustez. Em caso de evento (queda detectada ou botão de emergência), ele:

- Solicita a última localização ao módulo GPS.
- Dispara padrão sonoro no buzzer e sinaliza em LED.
- Envia mensagem ao Telegram com link do Google Maps (quando disponível).

A configuração de rede e nome do usuário é feita por um webserver no próprio dispositivo (Modo de Configuração). Após configurado, opera no Modo Normal, conectando ao Wi‑Fi e executando as tarefas em paralelo sob FreeRTOS. Um watchdog (software + hardware) monitora as tarefas e reinicia o sistema em caso de travamentos.

---

## ✨ Funcionalidades Principais

- Detecção de quedas por ML embarcado (Edge Impulse/TFLite).
- Botão de emergência dedicado para alerta manual.
- Notificações via Telegram (HTTPS) com link do Google Maps.
- GPS (ATGM336H) para anexar localização ao alerta.
- Buzzer para alerta sonoro local com padrões por evento.
- LEDs RGB para estados do sistema (inicialização, funcionando, erro, configuração).
- Servidor web de configuração (AP próprio) para SSID/senha e nome do usuário.
- Execução multitarefa com FreeRTOS e supervisão por watchdog.
- Persistência de configuração em Flash interna (sem cartão SD).

---

## 🧩 Arquitetura do Software

Tarefas FreeRTOS e responsabilidades (principais):
- LED Status (`LEDStatus`): indica o estado do sistema (fila `status_queue`).
- Watchdog Monitor (`WatchdogMonitor`): vigia heartbeats de tarefas e depende de watchdog HW (8 s) e SW (20 s por tarefa).
- Wi‑Fi Init (`WiFiInit`): inicializa CYW43, conecta à rede configurada e sinaliza semáforo de Wi‑Fi pronto.
- Sensor MPU6050 (`ReadAccelGyro`): lê acel/gyro via I2C periodicamente (20 ms) e enfileira amostras.
- Detecção de Quedas (`FallDetection`): agrega janelas e roda `run_classifier`, publicando eventos.
- Botão de Emergência (`EmergencyButton`): IRQ no GPIO com debounce, publica evento e dispara buzzer/GPS.
- Buzzer (`Buzzer`): executa padrões sonoros conforme eventos.
- GPS (`GPSLocation`): lê NMEA via UART, parseia (minmea) e responde com última posição.
- Rede/Telegram (`Network`): cliente HTTPS com Mongoose para POST na API do Telegram.
- Web Config (`ConfigWeb`): modo AP + HTTP em 192.168.4.1 para gravar SSID/senha/usuário em Flash.

Filas/Semáforos relevantes: `sensor_queue`, `event_queue`, `buzzer_queue`, `status_queue`, `gps_queue`, `gps_req`, `wifi_semaphore`, `gps_semaphore`.

Estados em LED (conforme `led.cpp`):
- Starting: azul constante.
- Working: verde constante.
- Error: vermelho constante.
- Config: azul piscando.

Eventos (conforme `types.h`):
- `EVENT_FALL_DETECTED` (label "Fall" - alerta).
- `EVENT_DAILY_ACTIVITY` (atividade normal — sem alerta).
- `EVENT_EMERGENCY_BUTTON_PRESSED`.

---

## 🔌 Hardware e Pinout

Placa alvo: Raspberry Pi Pico W

- I2C1 (MPU6050):
  - `GPIO 2` (SDA), `GPIO 3` (SCL), 3V3 e GND. Endereço 0x68.
- GPS ATGM336H (UART1, 9600 bps):
  - `GPIO 8` (TX do Pico → RX do GPS), `GPIO 9` (RX do Pico ← TX do GPS), 3V3 e GND.
- Buzzer passivo (PWM):
  - `GPIO 21`.
- Botão de Emergência (pull-up interno, aciona em nível baixo):
  - `GPIO 5` (ligar ao GND ao pressionar).
- Botão de Configuração (pressionar ao ligar para entrar em modo AP):
  - `GPIO 6` (ligar ao GND ao pressionar).
- LED RGB externo (catodo comum, nível alto acende conforme implementação):
  - Azul: `GPIO 12`
  - Vermelho: `GPIO 13`
  - Verde: `GPIO 11`

---

## 🛠️ Ambiente e Tecnologias

- Linguagens: C/C++
- RTOS: FreeRTOS (Kernel incluso no repositório)
- Conectividade: CYW43 (Pico W) + lwIP + Mongoose (cliente HTTP/HTTPS)
- ML: Edge Impulse SDK + modelo TFLite (pasta `model/`)
- Parser GPS: `minmea`
- SDK: Raspberry Pi Pico SDK (2.x)
- Build: CMake + toolchain ARM (arm-none-eabi)

---

## Dependências: FreeRTOS‑Kernel

Este projeto depende do repositório oficial do FreeRTOS‑Kernel com o port para RP2040. Caso a pasta `FreeRTOS-Kernel/` não exista ao lado do `CMakeLists.txt` faça:

Clone dentro de `Fall_Detection/`:

```bash
cd Fall_Detection
git clone --depth 1 https://github.com/FreeRTOS/FreeRTOS-Kernel.git FreeRTOS-Kernel
```

---

## 🚀 Compilar e Gravar

Pré‑requisitos:
- Pico SDK e toolchain ARM instalados e no PATH.
- CMake ≥ 3.13.
- Opcional: Extensão oficial Raspberry Pi Pico no VS Code.

Via terminal (Linux):

```bash
cd Fall_Detection
cmake -B build -S . -DPICO_BOARD=pico_w -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
# O UF2 será gerado em build/Fall_Detection.uf2
```

Gravação (BOOTSEL):
1) Conecte o Pico W em modo BOOTSEL (segurando o botão BOOTSEL ao plugar USB).
2) Monte a unidade USB exposta pelo Pico.
3) Copie `build/Fall_Detection.uf2` para a unidade.

Dica: para “zerar” a Flash (remover configuração salva), grave o `flash_nuke.uf2` oficial e depois regrave o firmware.

---

## 🔐 Configurar Telegram

Antes de compilar, ajuste Token e Chat ID do Telegram em `main.cpp`:

```c
#define TELEGRAM_BOT_TOKEN "SEU_TOKEN_AQUI"
#define TELEGRAM_CHAT_ID   "SEU_CHAT_ID_AQUI"
```

- Crie um bot com o @BotFather e obtenha o Token.
- Use um Chat ID de grupo/usuário que receberá os alertas.
- Evite commitar segredos no repositório público.

---

## 📶 Primeiro Uso: Modo de Configuração (AP)

O dispositivo entra em modo de configuração quando:
- O botão de configuração (`GPIO 6`) é mantido pressionado na inicialização, ou
- Não existe configuração válida na Flash.

Passos:
1. Mantenha o botão de configuração pressionado durante a inicialização (se desejar forçar o modo AP).
2. Conecte-se ao Wi‑Fi criado pelo dispositivo:
   - SSID: `FallDetector_Config`
   - Senha: `config123`
3. Abra no navegador: `http://192.168.4.1`.
4. Preencha e salve:
   - SSID e senha da sua rede Wi‑Fi 2,4 GHz.
   - Nome do usuário (quem usa o dispositivo, ex.: “Maria Silva”).
5. O dispositivo reinicia automaticamente após alguns segundos e tentará conectar ao Wi‑Fi.

### Configure IP manual

O cliente conectado ao AP não recebe IP automaticamente. Se ao digitar `http://192.168.4.1` a página não abrir, configure IPv4 manual na interface Wi‑Fi conectada ao SSID `FallDetector_Config`:

- Address: `192.168.4.2`
- Netmask: `255.255.255.0`
- Gateway: `192.168.4.1`
- DNS: `8.8.8.8` (ou mantenha automático)

Depois, acesse novamente `http://192.168.4.1` para salvar as credenciais. (No Ubuntu, isso corresponde a selecionar IPv4 “Manual” e preencher)

---

## ▶️ Operação Normal

- Após a conexão ao Wi‑Fi, as tarefas são iniciadas:
  - Leitura do MPU6050 a cada 20 ms.
  - Janela de dados alimenta o classificador Edge Impulse.
  - Em caso de detecção de queda ou botão de emergência:
    - Buzzer toca padrão específico.
  - GPS tenta obter uma sentença NMEA válida; se bem-sucedido, um link do Google Maps é incluído; caso contrário, “Unavailable”.
    - Mensagem é enviada ao Telegram via HTTPS (Mongoose + mbedTLS).

- Watchdog: se alguma tarefa deixar de reportar “vida”, o LED vai a erro e o watchdog de hardware reinicia o sistema.

---

## 🧪 Modelo de ML (Edge Impulse)

- O projeto inclui o SDK do Edge Impulse e o modelo TFLite em `model/`.
- A função `fall_detection_task` agrega amostras (ax, ay, az, gx, gy, gz) até `EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE` e chama `run_classifier`.
- Ajustes de modelo/dataset podem ser feitos fora deste firmware; scripts de processamento do dataset SisFall estão em `Machine_Learning_Scripts_Data/` na raiz do repositório.

---

## ❗ Solução de Problemas

- Não conecta ao Wi‑Fi:
  - Verifique SSID/senha (somente 2,4 GHz).
- Sem localização:
  - O GPS pode não obter fix em ambientes internos. O link pode sair como “Unavailable”.
  - Aguarde alguns minutos com visada do céu.
- Sistema reiniciando:
  - Watchdog pode estar atuando. Verifique alimentação, cabos e logs (USB/serial).
- Resetar configuração:
  - Grave `flash_nuke.uf2` e depois regrave o firmware para limpar a Flash.

---

## 📁 Estrutura

- `main.cpp`: inicialização, criação de filas/semaf., binding de tarefas e pinos.
- `src/` e `include/`: drivers (I2C/UART), tarefas (sensor, GPS, rede, botão, buzzer, LED, watchdog).
- `mongoose-minimal/`: Mongoose (HTTP/HTTPS client) + opções.
- `minmea/`: parser NMEA para GPS.
- `model/`: SDK Edge Impulse + modelo TFLite integrado ao build.

---

## 👤 Autores

- Luana Maria da Silva Menezes
- Vinicius de Souza Caffeu

---

## 📜 Licença

Consulte o arquivo `LICENSE` na raiz do repositório.
