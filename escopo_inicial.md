# Escopo Inicial - Projeto Bastão Unihiker

## 1. Visão Geral do Sistema
O objetivo do projeto é construir um "Bastão" inteligente para fins de monitoramento e controle. O cérebro principal será um **Unihiker K10**, rodando C/C++ via ESP-IDF diretamente no seu microcontrolador nativo ESP32-S3. Toda interface com o usuário, conectividade segura via rede (Mesh/TCP) e regras de negócio operarão de forma unificada no mesmo chip.

## 2. Padrões de Segurança (The "Gateway" Layer)
Apesar de ser um único dispositivo, o núcleo de rede atua como um *Gateway* seguro com as seguintes premissas obrigatórias:
- **Criptografia:** Toda comunicação trocada na rede será criptografada (ex: AES via mbedTLS).
- **Whitelist:** O firmware manterá um cadastro persistente (usando NVS) dos IDs/MACs dos ESPs de sensores (ex: sensor na árvore, sensor no colete, etc). Se a placa tentar se conectar na rede Mesh/TCP do Bastão e não constar na Whitelist, os pacotes são sumariamente rejeitados.

## 3. Interface Visual (As 4 Telas - LVGL)
A experiência do usuário será implementada em **LVGL (Light and Versatile Graphics Library)**, exibida nativamente na tela do K10 com quatro "rotas" principais ativadas por Toque (touchscreen):
1. **Dados**: Visualização das variáveis principais e dados dos sensores em tempo real.
2. **Conectividade**: Configuração e status das conexões (Mesh, pareamento seguro TCP, Wi-Fi).
3. **Servidor e Sensores**: Configurações avançadas de endpoints/servidor e gerenciamento e status individual de componentes sensoriais.
4. **Informações Gerais**: Métricas do sistema, bateria, versão, diagnóstico e logs essenciais.

* **Hardware Gateway (ESP32 Tela)**: Todo recebimento de dados advindos de outros módulos passa obrigatoriamente por este ESP32 principal. A tela Unihiker apenas exibe e configura, não se conecta à malha diretamente.
* **Topologia e Rede**: O ESP32 utiliza protocolo TCP ou Mesh para se comunicar com outros nós da rede.
* **Segurança e Criptografia**:
  * Comunicação *end-to-end* criptografada (ex: TLS ou AES-256 local sobre payload TCP/Mesh).
  * O ESP da tela possui uma política estrita de *whitelist*: ele só aceita conexões de ESPs cujo cadastro tenha sido realizado previamente no sistema.
  * Requisições de módulos não autorizados são ignoradas/bloqueadas no ato da recepção.

## 4. Pilha Tecnológica
* **ESP32 Firmware**: Espressif ESP-IDF estruturado com FreeRTOS.
* **Unihiker Application**: Utilizará Python, rodando um loop assíncrono para UI e recebimento de dados seriais/socket vindo do ESP32 principal.
* **Boas Práticas e Código**: 
  - Código altamente modular; cada responsabilidade é separada em bibliotecas independentes (`components` no IDF).
  - Todas as funções devem ser rigorosamente comentadas via Doxygen ou standards similares, explicando funcionamento, parâmetros e retornos.
  - O código deverá estar modelado para permitir *pivotagem* rápida de requisitos.
