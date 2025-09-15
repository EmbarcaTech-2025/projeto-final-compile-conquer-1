# Idoso Seguro: Módulo Detector de Gás com Pico W

Este repositório contém o firmware para o módulo de detecção de vazamento de gás do projeto "Idoso Seguro". O dispositivo utiliza uma Raspberry Pi Pico W para monitorar o ambiente em tempo real e tomar ações de segurança automáticas, tanto locais quanto remotas.

O projeto foi desenvolvido como parte do programa **Embarca Tech** do **Instituto Hardware BR**.

## 🎯 Visão Geral

O objetivo deste módulo é garantir a segurança de idosos em ambientes domésticos, permitindo que mantenham sua autonomia ao cozinhar. O sistema é projetado para ser "fail-safe", onde qualquer falha de energia ou do sistema resulta no estado mais seguro: o corte do fornecimento de gás.

A solução é composta por dois modos de operação: um **Modo de Configuração** para uma instalação fácil e um **Modo de Operação Normal** para monitoramento contínuo.

## ✨ Funcionalidades Principais

* **Configuração via Wi-Fi:** Na primeira inicialização, o dispositivo cria um Ponto de Acesso (AP) com uma página web para uma configuração simples das credenciais de rede e ID do dispositivo.
* **Detecção de Gás:** Monitoramento contínuo do ambiente com um sensor de gás MQ-2.
* **Ação de Segurança Local:** Ao detectar um vazamento, o sistema:
    * Aciona um **relé** para fechar uma válvula solenoide Normalmente Fechada (NF), cortando o fluxo de gás instantaneamente.
    * Dispara um **alarme sonoro** através de um buzzer passivo controlado por PWM.
* **Alerta Remoto:** Envia uma notificação imediata e específica via **HTTPS POST** para um chat do Telegram.
* **Sistema Robusto:** Utiliza **FreeRTOS** para gerenciamento de tarefas concorrentes e uma lógica de **watchdog** para recuperação automática de falhas.

## 🛠️ Arquitetura e Tecnologias

* **Microcontrolador:** Raspberry Pi Pico W
* **Sistema Operacional:** FreeRTOS
* **Biblioteca de Rede:** Mongoose Networking Library
* **Serviços de Rede (Modo AP):** Servidor DHCP da biblioteca LwIP e Servidor DNS da biblioteca Mongoose para implementação do portal cativo.
* **Persistência:** As configurações do usuário são salvas na memória Flash interna da Pico W.
* **Linguagem:** C
* **IDE:** Visual Studio Code com a extensão Raspberry Pi Pico

## 🚀 Como Compilar e Usar

### 1. Pré-requisitos
* Ambiente de desenvolvimento para Raspberry Pi Pico configurado (SDK, toolchain ARM, CMake).
* VS Code com a extensão oficial da Raspberry Pi.

### 2. Antes de Compilar: Configure o Telegram
> **IMPORTANTE:** O Token do Bot do Telegram e o Chat ID são fixos no código para maior segurança. Você **precisa** editá-los antes de compilar.

Abra o arquivo `main.c` e substitua os valores nas seguintes linhas:
```c
#define TELEGRAM_BOT_TOKEN  "SEU_TOKEN_AQUI"
#define TELEGRAM_CHAT_ID    "SEU_CHAT_ID_AQUI"
```

### 3. Compilação
* Clone este repositório.
* Abra a pasta do projeto no VS Code.
* A extensão CMake deve configurar o projeto automaticamente.
* Compile o projeto usando o botão **"Build"** na barra de status do VS Code (ou pressionando `F7`).

### 4. Primeira Utilização (Modo de Configuração)
1.  **Limpe a Memória:** Use o arquivo `flash_nuke.uf2` da Raspberry Pi para apagar completamente a memória flash da sua Pico W.
2.  **Grave o Firmware:** Coloque a Pico W em modo BOOTSEL novamente e arraste o arquivo `.uf2` compilado do seu projeto para ela.
3.  **Conecte-se à Rede de Configuração:**
    * No seu celular ou computador, procure e conecte-se à rede Wi-Fi:
        * **Nome da Rede (SSID):** `PicoGasSetup`
        * **Senha:** `config1234`
4.  **Acesse a Página Web:**
    * A página de configuração deve abrir automaticamente no seu navegador (portal cativo).
    * Se não abrir, acesse manualmente o endereço: **`http://19.168.4.1`**
5.  **Salve as Credenciais:**
    * Preencha os dados da sua rede Wi-Fi local e um nome de identificação para o dispositivo.
    * Clique em "Salvar e Reiniciar".

### 5. Uso Normal
* Após reiniciar, o dispositivo se conectará automaticamente ao Wi-Fi que você configurou e iniciará o monitoramento.
* Uma mensagem "Detector de Gás Online" será enviada ao seu Telegram para confirmar que tudo está funcionando.

---
**Autores:**
* Luana Maria da Silva Menezes
* Vinicius de Souza Caffeu
