/**
 * @file gui_theme.h
 * @brief Definições de estilo e cores para o projeto Bastão Unihiker K10.
 * Segue o padrão "Modern Dark" com foco em Agritech.
 */

#ifndef GUI_THEME_H
#define GUI_THEME_H

#include "lvgl.h"

/* Paleta de Cores (Modern Dark / Agritech) */
#define COLOR_EMERALD      lv_color_hex(0x2ECC71) // Verde Principal
#define COLOR_ANTHRACITE   lv_color_hex(0x2C3E50) // Fundo e Cards
#define COLOR_BG_DARK      lv_color_hex(0x1A1A1A) // Fundo Geral
#define COLOR_WHITE        lv_color_hex(0xFFFFFF) // Texto e Ícones
#define COLOR_GRAY_TEXT    lv_color_hex(0xBDC3C7) // Texto secundário

/* Parâmetros de Design */
#define GUI_RADIUS_DEFAULT 10
#define GUI_PADDING_S      5
#define GUI_PADDING_M      10
#define GUI_PADDING_L      20

/* Estilos Globais */
static inline void gui_style_init_card(lv_style_t *style) {
    lv_style_init(style);
    lv_style_set_bg_color(style, COLOR_ANTHRACITE);
    lv_style_set_radius(style, GUI_RADIUS_DEFAULT);
    lv_style_set_shadow_width(style, 10);
    lv_style_set_shadow_ofs_y(style, 5);
    lv_style_set_shadow_opa(style, LV_OPA_30);
    lv_style_set_text_color(style, COLOR_WHITE);
}

static inline void gui_style_init_btn_primary(lv_style_t *style) {
    lv_style_init(style);
    lv_style_set_bg_color(style, COLOR_EMERALD);
    lv_style_set_radius(style, 20); // Botões mais arredondados
    lv_style_set_text_color(style, COLOR_WHITE);
    lv_style_set_text_font(style, &lv_font_montserrat_14);
}

#endif // GUI_THEME_H
