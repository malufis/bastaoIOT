#include "gui_manager.h"
#include "gui_theme.h"
#include "esp_log.h"
#include "lvgl.h"
#include "hal_display.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include <stdio.h>

static const char *TAG = "GUI_MANAGER";

static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx) {
    lv_disp_drv_t *disp_driver = (lv_disp_drv_t *)user_ctx;
    lv_disp_flush_ready(disp_driver);
    return false;
}

#define LCD_H_RES 240
#define LCD_V_RES 320

/* Objetos Globais da UI */
static lv_obj_t *tv;
static lv_obj_t *t1, *t2, *t3, *t4;

/* Estilos Reutilizáveis */
static lv_style_t style_card;
static lv_style_t style_btn;

static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map) {
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)drv->user_data;
    int x1 = area->x1;
    int y1 = area->y1;
    int x2 = area->x2;
    int y2 = area->y2;
    // Transfere o buffer para o LCD via DMA
    esp_lcd_panel_draw_bitmap(panel_handle, x1, y1, x2 + 1, y2 + 1, color_map);
    // A função lv_disp_flush_ready será chamada pelo *callback do DMA* (notify_lvgl_flush_ready)
    // Desligamos isso aqui para evitar rasgar a tela durante a transferência assíncrona!
}

// Callback chamado pelo driver do LCD quando a transferência termina (opcional para sincronização)
// No momento simplificaremos sem ele ou usando o lv_disp_flush_ready direto no flush se for síncrono.
/**
 * @brief Cria a tela de Dashboard (Coleta de Dados)
 */
static void create_screen_dashboard(lv_obj_t *parent) {
    lv_obj_set_style_bg_color(parent, COLOR_BG_DARK, 0);

    /* Header */
    lv_obj_t *header = lv_label_create(parent);
    lv_label_set_text(header, "FAZENDA MODELO");
    lv_obj_set_style_text_font(header, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(header, COLOR_GRAY_TEXT, 0);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 10);

    /* Card Principal (ID do Brinco) */
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, 200, 100);
    lv_obj_add_style(card, &style_card, 0);
    lv_obj_align(card, LV_ALIGN_CENTER, 0, -20);

    lv_obj_t *id_label = lv_label_create(card);
    lv_label_set_text(id_label, "ID: 4509 - S");
    lv_obj_set_style_text_font(id_label, &lv_font_montserrat_14, 0); // Alterado de 20 para 14
    lv_obj_align(id_label, LV_ALIGN_CENTER, 0, 0);

    /* Botão de Scan (Floating Action Button style) */
    lv_obj_t *btn_scan = lv_btn_create(parent);
    lv_obj_set_size(btn_scan, 120, 45);
    lv_obj_add_style(btn_scan, &style_btn, 0);
    lv_obj_align(btn_scan, LV_ALIGN_BOTTOM_MID, 0, -20);

    lv_obj_t *btn_label = lv_label_create(btn_scan);
    lv_label_set_text(btn_label, LV_SYMBOL_VIDEO " SCAN");
    lv_obj_align(btn_label, LV_ALIGN_CENTER, 0, 0);
}

/**
 * @brief Cria a tela de Conectividade
 */
static void create_screen_iot(lv_obj_t *parent) {
    lv_obj_set_style_bg_color(parent, COLOR_BG_DARK, 0);

    lv_obj_t *list = lv_list_create(parent);
    lv_obj_set_size(list, 220, 180);
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_set_style_bg_color(list, COLOR_ANTHRACITE, 0);
    lv_obj_set_style_border_width(list, 0, 0);

    lv_list_add_text(list, "NETWORKS");
    lv_list_add_btn(list, LV_SYMBOL_WIFI, "Fazenda_WiFi: 75%");
    lv_list_add_btn(list, LV_SYMBOL_BLUETOOTH, "Bastão V3: Pareado");
    lv_list_add_btn(list, LV_SYMBOL_UPLOAD, "Cloud Sync: OK");
}

/**
 * @brief Cria a tela de Configurações
 */
static void create_screen_settings(lv_obj_t *parent) {
    lv_obj_set_style_bg_color(parent, COLOR_BG_DARK, 0);

    lv_obj_t *title = lv_label_create(parent);
    lv_label_set_text(title, "SISTEMA");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    /* Slider de Brilho */
    lv_obj_t *slider = lv_slider_create(parent);
    lv_obj_set_width(slider, 180);
    lv_obj_align(slider, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_bg_color(slider, COLOR_EMERALD, LV_PART_INDICATOR);
}

/**
 * @brief Cria a tela de Analytics (Gráficos)
 */
static void create_screen_analytics(lv_obj_t *parent) {
    lv_obj_set_style_bg_color(parent, COLOR_BG_DARK, 0);

    lv_obj_t *chart = lv_chart_create(parent);
    lv_obj_set_size(chart, 200, 150);
    lv_obj_align(chart, LV_ALIGN_CENTER, 0, 0);
    lv_chart_set_type(chart, LV_CHART_TYPE_LINE);
    
    lv_chart_series_t *ser = lv_chart_add_series(chart, COLOR_EMERALD, LV_CHART_AXIS_PRIMARY_Y);
    lv_chart_set_next_value(chart, ser, 10);
    lv_chart_set_next_value(chart, ser, 30);
    lv_chart_set_next_value(chart, ser, 20);
    lv_chart_set_next_value(chart, ser, 50);
}

void gui_manager_init(void) {
    ESP_LOGI(TAG, "Inicializando LVGL...");
    lv_init();

    esp_lcd_panel_handle_t panel_handle = hal_display_get_panel_handle();

    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf[240 * 40]; // Aumentado para 40 linhas para reduzir artefatos
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, 240 * 40);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 240;
    disp_drv.ver_res = 320;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.user_data = panel_handle;
    lv_disp_drv_register(&disp_drv);

    ESP_LOGI(TAG, "Registrando sincronismo V-SYNC do DMA...");
    esp_lcd_panel_io_handle_t io_handle = hal_display_get_io_handle();
    const esp_lcd_panel_io_callbacks_t cbs = {
        .on_color_trans_done = notify_lvgl_flush_ready,
    };
    // Atribui o disp_drv como parâmetro pro callback saber a quem avisar
    esp_lcd_panel_io_register_event_callbacks(io_handle, &cbs, &disp_drv);

    ESP_LOGI(TAG, "Configurando Estilos...");
    gui_style_init_card(&style_card);
    gui_style_init_btn_primary(&style_btn);

    ESP_LOGI(TAG, "Construindo Navegação...");
    
    /* TabView em baixo */
    tv = lv_tabview_create(lv_scr_act(), LV_DIR_BOTTOM, 50);
    lv_obj_set_style_bg_color(tv, COLOR_BG_DARK, 0);
    
    /* Configuração da Barra de Abas */
    lv_obj_t *tab_btns = lv_tabview_get_tab_btns(tv);
    lv_obj_set_style_bg_color(tab_btns, COLOR_ANTHRACITE, 0);
    lv_obj_set_style_text_color(tab_btns, COLOR_GRAY_TEXT, 0);
    lv_obj_set_style_bg_color(tab_btns, COLOR_EMERALD, LV_PART_ITEMS | LV_STATE_CHECKED);

    /* Criação das Abas */
    t1 = lv_tabview_add_tab(tv, LV_SYMBOL_HOME);
    t2 = lv_tabview_add_tab(tv, LV_SYMBOL_WIFI);
    t3 = lv_tabview_add_tab(tv, LV_SYMBOL_SETTINGS);
    t4 = lv_tabview_add_tab(tv, LV_SYMBOL_LIST); // Alterado de GRAPH para LIST

    create_screen_dashboard(t1);
    create_screen_iot(t2);
    create_screen_settings(t3);
    create_screen_analytics(t4);

    ESP_LOGI(TAG, "Interface pronta.");
}
