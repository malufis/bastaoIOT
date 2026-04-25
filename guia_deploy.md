# Guia de Deploy - Unihiker K10 (Firmware C/C++)

Este documento explica como gravar o firmware do Bastão Inteligente diretamente no **Unihiker K10**, utilizando o **ESP-IDF** (versão 5+). 
Como o K10 é baseado no **ESP32-S3** e estamos rodando C puro com FreeRTOS e LVGL, não utilizamos Linux, SSH ou Python para rodar nossa interface principal.

## Pré-requisitos
- Ter o **ESP-IDF SDK 5.x** instalado no Windows.
- O Unihiker K10 deve estar conectado via **cabo USB-C** ao computador.
- Você precisa identificar qual a porta **COM** ele criou no Gerenciador de Dispositivos do Windows (ex: COM3, COM4).

## Passo a Passo (Build & Flash)
1. No seu Windows, abra o terminal do **ESP-IDF 5.x PowerShell** ou ESP-IDF CMD.
2. Navegue até a pasta do novo firmware:
   ```powershell
   cd d:\git\Bastao\bastao-Unihker\k10_firmware
   ```
3. **Setar o Alvo (Target)** para o ESP32-S3:
   ```powershell
   idf.py set-target esp32s3
   ```
4. **Abrir o Menu de Configuração** *(Opcional, mas útil para Memory/PSRAM config)*:
   ```powershell
   idf.py menuconfig
   ```
5. **Compilar o código**:
   ```powershell
   idf.py build
   ```
6. **Gravar na memória do K10 (Flash)**:
   Substitua `COMX` pela sua porta real.
   ```powershell
   idf.py -p COMX flash
   ```
7. **Ver os logs (Monitor)**:
   ```powershell
   idf.py -p COMX monitor
   ```
   *(Pressione `Ctrl + ]` para sair do monitor)*

> [!TIP]
> Se a tela piscar ou houver travamentos no K10, a primeira coisa a verificar é o tamanho do heap e PSRAM disponíveis via `menuconfig`, já que a biblioteca gráfica LVGL puxa bastante memória para seus buffers.
