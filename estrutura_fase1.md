# Estrutura do Projeto - K10 Firmware (Opção B)

Toda a arquitetura do Bastão Unihiker K10 é concentrada em um único repositório nativo ESP-IDF (C/C++), uma vez que o dispositivo aproveita seu chip interno ESP32-S3 para desenhar a interface gráfica via **LVGL**, além de controlar criptografia e rede.

## 1. Topologia de Diretórios
```text
/bastao-Unihker
  ├── /k10_firmware       # Projeto nativo em C (ESP-IDF)
  │    ├── CMakeLists.txt # Arquivo base de build master
  │    ├── /main          # Application Entry Point
  │    │    ├── main.c    # Init tasks, FreeRTOS, NVS Flash   
  │    │    └── CMakeLists.txt
  │    └── /components    # Bibliotecas locais e modularizadas
  │         ├── /gui      # A UI (4 telas do LVGL e gerenciamento de estado)
  │         ├── /hal      # Hardware Abstraction: Drivers ST7789(Display), CST816(Touch)
  │         ├── /comm     # Comunicação TCP/Mesh e Cadastro de ESPs autorizados
  │         ├── /crypto   # Motor de criptografia / descriptografia (mbedTLS)
  │         └── /utils    # Funções genéricas de logging, helpers, etc.
  │
  └── /.agents            # Prompts/Contextos dos agentes locais especialistas
```

## 2. Padrões de Código e Diretrizes (FreeRTOS)
- **Modularidade de Threads**: As operações intensivas de rede (`/comm`) e descriptografia (`/crypto`) rodam na arquitetura **Dual-Core do S3** (no `PRO_CPU`). A interface visual e o preenchimento de buffers da LVGL (`/gui`) ficam no `APP_CPU` para evitar que a tela engasgue a 60fps.
- **Isolamento via HAL**: O diretório `/gui` não conversa diretamente com pinos SPI ou I2C do display/touch. Todo acesso ao hardware é encapsulado pela `/hal`.
- **Docs Integrados**: TODA função exposta num `.h` tem *docstring* Doxygen.
