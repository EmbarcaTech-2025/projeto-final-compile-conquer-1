# 🧓 Projeto Idoso Seguro para Familias e Casas de Repouso

## Sobre o Projeto

O Idoso Seguro é uma solução tecnológica voltada para famílias e instituições que cuidam de idosos, com o objetivo de aumentar a segurança, autonomia e qualidade do cuidado. O sistema permite que poucos cuidadores monitorem vários idosos simultaneamente, centralizando informações essenciais e agilizando respostas em situações de emergência.

A plataforma integra dois dispositivos principais:

- **Monitoramento corporal inteligente:** detecção automática de quedas com machine learning embarcado, alertas manuais e automáticos, além de sinalização clara de falhas técnicas. Isso reduz custos operacionais, facilita o trabalho dos profissionais e amplia a cobertura do cuidado, tornando o ambiente mais seguro e eficiente.
- **Monitoramento de gases inflamáveis:** utiliza sensores para detectar vazamentos de gás em ambientes domésticos, especialmente na cozinha, acionando notificações automáticas e atuadores para prevenir acidentes. Essa funcionalidade é fundamental para evitar riscos associados ao uso do fogão por idosos, proporcionando uma camada extra de proteção.

O projeto foi desenvolvido para ser fácil de instalar, configurar e utilizar, tanto em residências quanto em casas de repouso, promovendo um ambiente mais protegido e inteligente para idosos e tranquilidade para familiares e cuidadores.

---

## Dispositivo de Monitoramento Corporal

Este protótipo demonstra as principais funcionalidades do sistema de monitoramento para idosos em casa ou casas de repouso.

### ✅ Funcionalidades Implementadas

- **Detecção de quedas com machine learning embarcado**  
  Utiliza modelo de aprendizado de máquina rodando diretamente no dispositivo para identificar quedas.
- **Gerenciamento multitarefa com RTOS**  
  O sistema utiliza um RTOS para garantir execução eficiente e simultânea das tarefas críticas (sensores, notificações, atuadores).
- **Watchdog para monitoramento das tarefas**  
  Um watchdog supervisiona continuamente as tarefas do sistema, reiniciando automaticamente o dispositivo em caso de falhas ou travamentos, aumentando a confiabilidade.
- **Webserver integrado para configuração**  
  Interface web intuitiva permite cadastrar o nome do usuário e inserir as credenciais da rede Wi-Fi, facilitando a instalação e personalização do dispositivo.
- **Sistema de LEDs para sinalização de status e erros**  
  LEDs indicam o funcionamento normal, inicialização e erros críticos.
- **Notificação via Telegram** (Wi-Fi) para os cuidadores responsáveis ao detectar queda e ao pressionar o botão de emergência.
- **Botão de emergência** para envio de alerta manual.
- **Alerta sonoro** via buzzer em caso de emergência ou queda.


### 🎥 Vídeo Demonstrativo

Confira o vídeo abaixo para ver o protótipo em funcionamento:

[Assista ao vídeo demonstrativo]()

---

## Dispositivo de Monitoramento de Gases

Este projeto implementa um sistema de monitoramento de vazamento de gases inflamáveis utilizando o sensor **MQ-02** integrado a um **Raspberry Pi Pico W**.  
O sistema tem como objetivo aumentar a segurança de idosos ao utilizar o fogão, detectando a presença de gases perigosos e reagindo automaticamente para evitar acidentes.  

### ✅ Funcionalidades Implementadas

- **Leitura do sensor MQ-02 para monitorar a presença de gases inflamáveis**  
  Testado e calibrado a partir de um potenciômetro disponível no módulo no MQ-02 capaz de regular o threshold do sensor. Alimentado por uma fonte externa de 5V e a   saída digital utilizada para o monitoramento do gás, foi aplicada em um divisor de tensão para não danificar as GPIO's da placa. A referência comum (GND) da        BitDogLab e da fonte externa foram interligadas.
- **Acionamento do módulo relé 3,3V**  
  O relé tem seu acionamento a partir da detecção de nível de gás para controle da válvula solenóide.
- **Sistema de notificação via Telegram**
  Para alerta da deteção de vazamentos.


### 🎥 Vídeo Demonstrativo

[Assista ao vídeo demonstrativo]()
