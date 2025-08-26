# 🧓 Projeto Idoso Seguro para Casas de Repouso – Etapa 3


## Sobre o Projeto

Este projeto foi desenvolvido para transformar a rotina de casas de repouso, permitindo que um número reduzido de cuidadores monitore simultaneamente diversos idosos de forma eficiente e segura. Ao centralizar o acompanhamento dos usuários, o sistema reduz custos operacionais, aumenta a autonomia dos idosos e garante respostas rápidas em situações de emergência.

A solução monitora continuamente o estado corporal de cada usuário, detectando quedas com machine learning embarcado, permitindo alertas manuais e sinalizando falhas técnicas de forma clara e automática. O foco é facilitar o trabalho dos profissionais, ampliar a cobertura do cuidado e tornar o ambiente institucional mais seguro e inteligente.

---

## Dispositivo de Monitoramento Corporal - Protótipo Funcional

Este protótipo demonstra as principais funcionalidades do sistema de monitoramento para idosos em casas de repouso. O objetivo é validar a integração dos sensores, atuadores e comunicação básica antes da versão final.

### ✅ Funcionalidades Implementadas

- **Detecção de quedas com machine learning embarcado**  
  Utiliza modelo de aprendizado de máquina rodando diretamente no dispositivo para identificar quedas.
- **Gerenciamento multitarefa com RTOS**  
  O sistema utiliza um RTOS para garantir execução eficiente e simultânea das tarefas críticas (sensores, notificações, atuadores).
- **Sistema de LEDs para sinalização de status e erros**  
  LEDs indicam o funcionamento normal, inicialização e erros críticos, com reinício automático do dispositivo em caso de falha grave.
- **Notificação via Telegram** (Wi-Fi) para os cuidadores responsáveis ao detectar queda e ao pressionar o botão de emergência.
- **Botão de emergência** para envio de alerta manual.
- **Alerta sonoro** via buzzer em caso de emergência ou queda.


### 🎥 Vídeo Demonstrativo

Confira o vídeo abaixo para ver o protótipo em funcionamento, incluindo a detecção de quedas com machine learning embarcado, acionamento do botão de emergência, LEDs para sinalização de status e erros, e envio de notificações:

[Assista ao vídeo demonstrativo](https://youtu.be/6EJemtafqjY)

### 🛠️ Ajustes Planejados para a Versão Final

- Adicionar suporte a **GSM** para envio de alertas sem Wi-Fi
- Integrar **GPS** para localização precisa em emergências
- Implementar página web local para configuração de Wi-Fi e cadastro do nome do idoso
- Modularizar mais o código para facilitar manutenção e expansão

---

## Dispositivo de Monitoramento de Gases -  Protótipo Funcional 

Este projeto implementa um sistema de monitoramento de vazamento de gases inflamáveis utilizando o sensor **MQ-02** integrado a um **Raspberry Pi Pico W**.  
O sistema tem como objetivo aumentar a segurança de idosos ao utilizar o fogão, detectando a presença de gases perigosos e reagindo automaticamente para evitar acidentes.  

### ✅ Funcionalidades Implementadas

- **Leitura do sensor MQ-02 para monitorar a presença de gases inflamáveis**  
  Testado e calibrado a partir de um potenciômetro disponível no módulo no MQ-02 capaz de regular o threshold do sensor. Alimentado por uma fonte externa de 5V e a   saída digital utilizada para o monitoramento do gás, foi aplicada em um divisor de tensão para não danificar as GPIO's da placa. A referência comum (GND) da        BitDogLab e da fonte externa foram interligadas.
- **Acionamento do módulo relé 3,3V**  
  O relé tem seu acionamento a partir da detecção de nível de gás para controle da válvula solenóide

### 🎥 Vídeo Demonstrativo

Adicionar aqui

### 🛠️ Ajustes Planejados para a Versão Final

-Adicionar o sistema de notificação via Telegram da deteção de vazamentos.
-Adicionar a válvula solenóide na saída do relé junto a um diodo de roda livre para proteção contra sobrecarga sob o relé.
