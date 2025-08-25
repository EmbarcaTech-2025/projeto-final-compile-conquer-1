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

Adicionar aqui 

### ✅ Funcionalidades Implementadas

Adicionar aqui 

### 🎥 Vídeo Demonstrativo

Adicionar aqui

### 🛠️ Ajustes Planejados para a Versão Final

Adicionar aqui