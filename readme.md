# 🧓 Projeto Idoso Seguro

## Sobre

O Idoso Seguro é um ecossistema de soluções tecnológicas voltadas para famílias e instituições que cuidam de idosos, com o objetivo de aumentar a segurança, a autonomia e a qualidade do cuidado. O sistema permite que cuidadores monitorem um ou vários idosos simultaneamente, centralizando informações essenciais e agilizando respostas em situações de emergência.

A plataforma integra dois dispositivos principais:

- **Monitoramento corporal inteligente:** detecta quedas com machine learning embarcado (TensorFlow Lite), reduzindo a necessidade de vigilância contínua. Envia alertas imediatos via Telegram, inclui localização GPS para emergências e oferece configuração simples por webserver. Com FreeRTOS e watchdog, mantém operação contínua e confiável, reduz custos operacionais e permite que um cuidador acompanhe múltiplos idosos com mais segurança.

- **Monitoramento de gases inflamáveis:** mitiga um dos principais riscos domésticos para idosos ao monitorar vazamentos de GLP com o sensor MQ-2. Ao identificar risco, fecha automaticamente a válvula via relé, aciona alarmes e notifica em tempo real pelo Telegram. A configuração via Wi‑Fi é simples e, com FreeRTOS e recuperação automática, o sistema opera 24/7, tornando a cozinha significativamente mais segura.

O projeto foi desenvolvido para ser fácil de instalar, configurar e utilizar, tanto em residências quanto em casas de repouso, promovendo um ambiente mais protegido e inteligente para idosos e tranquilidade para familiares e cuidadores.

---

## Dispositivo de Monitoramento Corporal

Principais funcionalidades do sistema de monitoramento corporal para idosos em casa ou casas de repouso.

### ✅ Funcionalidades Implementadas

- **Detecção de quedas com machine learning embarcado**
  Utiliza um modelo rodando diretamente no dispositivo para identificar quedas com baixa latência.
- **Botão de emergência** para envio de alerta manual.
- **Notificação via Telegram (Wi‑Fi)** para cuidadores ao detectar queda ou pressionar o botão de emergência.
- **Rastreamento GPS e envio de localização em alertas.**  
  Integração com o módulo ATGM336H para fornecer coordenadas em tempo real e incluí‑las nas notificações.
- **Alerta sonoro** via buzzer em caso de emergência ou queda.
- **Sistema de LEDs para sinalização de status e erros.**  
  Indicação de funcionamento normal, inicialização e erros críticos.
- **Servidor web integrado para configuração.**  
  Interface web para cadastrar o nome do usuário e inserir as credenciais da rede Wi‑Fi.
- **Gerenciamento multitarefa com RTOS.**  
  Execução eficiente e simultânea das tarefas críticas (sensores, notificações, atuadores).
- **Watchdog para monitoramento das tarefas.**  
  Reinicia automaticamente o dispositivo em caso de falhas ou travamentos, aumentando a confiabilidade.


### 🎥 Vídeo Demonstrativo

Confira o vídeo abaixo para ver o protótipo em funcionamento:

[Assista ao vídeo demonstrativo do Dispositivo de Monitoramento Corporal](https://youtu.be/jmjdDbjIJ9U)


### Documentação do Dispositivo de Monitoramento Corporal

Cada dispositivo possui um README próprio dentro da pasta do código, com instruções de hardware, compilação, configuração e uso:

- Código-fonte: [./Fall_Detection](./Fall_Detection/)
- Documentação detalhada: [./Fall_Detection/readme.md](./Fall_Detection/readme.md)

---

## Dispositivo de Monitoramento de Gases

Este projeto implementa um sistema de monitoramento de vazamento de gases inflamáveis utilizando o sensor **MQ-02** integrado a um **Raspberry Pi Pico W**.  
O sistema tem como objetivo aumentar a segurança de idosos ao utilizar o fogão, detectando a presença de gases perigosos e reagindo automaticamente para evitar acidentes.  

### ✅ Funcionalidades Implementadas

* **Configuração via Wi-Fi:** Na primeira inicialização, o dispositivo cria um Ponto de Acesso (AP) com uma página web para uma configuração simples das credenciais de rede e ID do dispositivo.
* **Detecção de Gás:** Monitoramento contínuo do ambiente com um sensor de gás MQ-2.
* **Ação de Segurança Local:** Ao detectar um vazamento, o sistema:
    * Aciona um **relé** para fechar uma válvula solenoide Normalmente Fechada (NF), cortando o fluxo de gás instantaneamente.
    * Dispara um **alarme sonoro** através de um buzzer passivo controlado por PWM.
* **Alerta Remoto:** Envia uma notificação imediata e específica via **HTTPS POST** para um chat do Telegram.
* **Sistema Robusto:** Utiliza **FreeRTOS** para gerenciamento de tarefas concorrentes e uma lógica de **watchdog** para recuperação automática de falhas.


### 🎥 Vídeo Demonstrativo

Confira o vídeo abaixo para ver o protótipo em funcionamento:

[Assista ao vídeo demonstrativo do Dispositivo de Monitoramento de Gases]()


### Documentação do Dispositivo de Monitoramento de Gases

Cada dispositivo possui um README próprio dentro da pasta do código, com instruções de hardware, compilação, configuração e uso:

- Código-fonte: [./gas_sensor_http](./gas_sensor_http/)
- Documentação detalhada: [./gas_sensor_http/README.md](./gas_sensor_http/README.md)
