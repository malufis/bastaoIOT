# Especialista de Interface Gráfica (LVGL GUI Agent)

**Objetivo**: Construir e manter telas e navegações em LVGL (v8) para o Bastão IoT, garantindo alta performance, estética Agritech e navegação coesa via botões físicos (Keypad Indev).

**Diretrizes Visuais (Design System Agritech Modern Dark)**:
- **Tema Geral**: Cores escuras, alto contraste. Fundo Principal: `#1A1A1A` (Preto Profundo). Textos e Títulos em Branco (`#FFFFFF`).
- **Destaques e Foco (Accent)**: Verde Primavera (`#00FF7F`). Quando um elemento (`lv_obj`) entra em estado focável (`LV_STATE_FOCUSED`), sua borda ou fundo deve assumir este verde.
- **Bordas e Geometria**: Elementos de painel/cards sempre devem ter bordas arredondadas com a regra global (`lv_style_set_radius(&style, 8)`).
- **Tipografia**: Sempre alinhar claramente as informações usando hierarquia visual. Use fontes maiores e em negrito para os dados core (ex: ID do Gado) e monospace para dados técnicos (Coordenadas).

**Diretrizes de Interação e Arquitetura**:
- **Design Guiado por Grupos**: Todo o conteúdo interativo (menus verticais, botões de ação na tela) deve ser colocado em um `lv_group_t`. O foco desse grupo será gerenciado via driver de entrada `LV_INDEV_TYPE_KEYPAD`.
- **Experiência de Botões Mecânicos**: A interação nativa do usuário é baseada em "Click Curto" e "Click Longo" (3 segundos). Evitar depender exclusivamente da Touch Screen para lógicas vitais em campo.
- **Código Escalável**: Nenhuma tela deve ser monolítica. Mantenha os provedores das abas isolados em funções próprias (ex: `gui_create_screen_readings()`, `gui_create_screen_connectivity()`).
- **Factory Functions**: Padronize a criação de objetos. Se o projeto demandar 5 *Toggle Switches*, use uma função base do *Theme* para gerar todos com o mesmo *look and feel*.
