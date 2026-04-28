#include "gui_manager.h"
#include "gui_theme.h"
#include "esp_log.h"
#include "lvgl.h"
#include "hal_display.h"
#include "hal_buttons.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include <stdio.h>

static const char *TAG = "GUI_MANAGER";

static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx) {
    lv_disp_drv_t *disp_driver = (lv_disp_drv_t *)user_ctx;
    lv_disp_flush_ready(disp_driver);
    return false;
}

#define LCD_V_RES 320

/* Protótipos de Funções Internas */
static void enter_screen_focus(void);
static void exit_screen_focus(void);
static void internal_obj_event_cb(lv_event_t * e);

/* Objetos Globais da UI */
static lv_obj_t *tv;
static lv_obj_t *t1, *t2, *t3, *t4;
static lv_group_t * g_main;

static void enter_screen_focus(void) {
    uint16_t act_tab = lv_tabview_get_tab_act(tv);
    lv_obj_t * target_obj = NULL;
    
    if(act_tab == 0) target_obj = (lv_obj_t *)lv_obj_get_user_data(t1);
    else if(act_tab == 1) target_obj = (lv_obj_t *)lv_obj_get_user_data(t2);
    else if(act_tab == 2) target_obj = (lv_obj_t *)lv_obj_get_user_data(t3);
    else if(act_tab == 3) target_obj = (lv_obj_t *)lv_obj_get_user_data(t4);

    if(target_obj) {
        lv_group_remove_all_objs(g_main);
        
        // Se for uma lista ou container, adiciona os filhos ao grupo para navegação
        uint32_t i;
        for(i = 0; i < lv_obj_get_child_cnt(target_obj); i++) {
            lv_obj_t * child = lv_obj_get_child(target_obj, i);
            lv_group_add_obj(g_main, child);
        }
        
        // Se não tiver filhos, adiciona o próprio objeto
        if(lv_obj_get_child_cnt(target_obj) == 0) {
            lv_group_add_obj(g_main, target_obj);
        }
        
        lv_group_focus_next(g_main); // Foca no primeiro item
    }
}

static void exit_screen_focus(void) {
    lv_group_remove_all_objs(g_main);
    lv_group_add_obj(g_main, tv);
    lv_group_focus_obj(tv);
}

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

static void create_screen_readings(lv_obj_t * parent) {
    lv_obj_set_style_bg_color(parent, COLOR_BG_DARK, 0);
    lv_obj_set_flex_flow(parent, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(parent, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // O user_data do parent guardará o objeto que deve receber o foco inicial
    // Será definido no final desta função (ex: btn_sync)

    // --- LINHA SUPERIOR (Status) ---
    // ... restante da tela ...
    lv_obj_t * header = lv_obj_create(parent);
    lv_obj_set_size(header, lv_pct(100), 20);
    lv_obj_set_style_bg_opa(header, 0, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    
    lv_obj_t * status = lv_label_create(header);
    lv_label_set_text(status, LV_SYMBOL_WIFI " " LV_SYMBOL_BLUETOOTH " " LV_SYMBOL_GPS " " LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_text_color(status, COLOR_EMERALD, 0);
    lv_obj_align(status, LV_ALIGN_RIGHT_MID, -10, 0);

    // --- ÁREA CENTRAL (Dados - Altura 5) ---
    lv_obj_t * center = lv_obj_create(parent);
    lv_obj_set_size(center, lv_pct(95), 160);
    lv_obj_add_style(center, &style_card, 0);
    
    lv_obj_t * farm_title = lv_label_create(center);
    lv_label_set_text(farm_title, "Fazenda Raptor");
    lv_obj_set_style_text_color(farm_title, COLOR_GRAY_TEXT, 0);
    lv_obj_align(farm_title, LV_ALIGN_TOP_MID, 0, 5);

    lv_obj_t * bovine_id = lv_label_create(center);
    lv_label_set_text(bovine_id, "Boi Bandido");
    lv_obj_set_style_text_font(bovine_id, &lv_font_montserrat_16, 0); 
    lv_obj_align(bovine_id, LV_ALIGN_CENTER, 0, -10);

    lv_obj_t * time_label = lv_label_create(center);
    lv_label_set_text(time_label, "25/04/2026 - 07:15");
    lv_obj_align(time_label, LV_ALIGN_BOTTOM_MID, 0, -25);

    lv_obj_t * gps_coords = lv_label_create(center);
    lv_label_set_text(gps_coords, "-20.444203, -54.619443");
    lv_obj_set_style_text_color(gps_coords, COLOR_GRAY_TEXT, 0);
    lv_obj_align(gps_coords, LV_ALIGN_BOTTOM_MID, 0, -5);

    // --- LINHA INFERIOR (Sensores - Altura 2) ---
    lv_obj_t * footer = lv_obj_create(parent);
    lv_obj_set_size(footer, lv_pct(100), 40);
    lv_obj_set_style_bg_opa(footer, 0, 0);
    lv_obj_set_style_border_width(footer, 0, 0);

    lv_obj_t * telemetry = lv_label_create(footer);
    lv_label_set_text(telemetry, "Inc: X:1.2 Y:0.5 | Bat: 3.8V");
    lv_obj_set_style_text_color(telemetry, COLOR_GRAY_TEXT, 0);
    lv_obj_align(telemetry, LV_ALIGN_TOP_MID, 0, 0);

    // Botão removido para evitar sobreposição no rodapé
    lv_obj_set_user_data(parent, NULL);
}

static void create_screen_connectivity(lv_obj_t * parent) {
    lv_obj_set_style_bg_color(parent, COLOR_BG_DARK, 0);
    lv_obj_t * list = lv_list_create(parent);
    lv_obj_set_size(list, lv_pct(100), lv_pct(95));
    lv_obj_align(list, LV_ALIGN_BOTTOM_MID, 0, 0);
    
    // Adiciona itens de exemplo para testar a navegação vertical
    lv_obj_t * btn;
    btn = lv_list_add_btn(list, LV_SYMBOL_WIFI, "Rede Fazenda_2G");
    lv_obj_add_event_cb(btn, internal_obj_event_cb, LV_EVENT_KEY, NULL);
    
    btn = lv_list_add_btn(list, LV_SYMBOL_WIFI, "Rede Sede_5G");
    lv_obj_add_event_cb(btn, internal_obj_event_cb, LV_EVENT_KEY, NULL);
    
    btn = lv_list_add_btn(list, LV_SYMBOL_SETTINGS, "Configurar IP");
    lv_obj_add_event_cb(btn, internal_obj_event_cb, LV_EVENT_KEY, NULL);

    lv_obj_set_user_data(parent, list); // Define o objeto principal para foco
}

static void create_screen_device(lv_obj_t * parent) {
    lv_obj_set_style_bg_color(parent, COLOR_BG_DARK, 0);
    // ... restante do device ...
}

static void create_screen_info(lv_obj_t * parent) {
    lv_obj_set_style_bg_color(parent, COLOR_BG_DARK, 0);
    // ... restante da info ...
}

static void tabview_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t key = lv_event_get_key(e);

    if(code == LV_EVENT_KEY) {
        ESP_LOGI(TAG, "Tecla recebida no TabView: %d", (int)key);
        if(key == LV_KEY_RIGHT) {
            uint16_t tab = lv_tabview_get_tab_act(tv);
            if(tab < 3) lv_tabview_set_act(tv, tab + 1, LV_ANIM_ON);
        } else if(key == LV_KEY_LEFT) {
            uint16_t tab = lv_tabview_get_tab_act(tv);
            if(tab > 0) lv_tabview_set_act(tv, tab - 1, LV_ANIM_ON);
        } else if(key == LV_KEY_ENTER) {
            ESP_LOGI(TAG, "Botão A 3s detectado: Entrando no foco da tela interna");
            enter_screen_focus();
        }
    }
}

static void internal_obj_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_KEY) {
        uint32_t key = lv_event_get_key(e);
        if(key == LV_KEY_ESC) {
            ESP_LOGI(TAG, "Botão B 3s detectado: Saindo da tela interna");
            exit_screen_focus();
        } else if(key == LV_KEY_LEFT) {
            // Tradução dinâmica: No Nível 2, Esquerda vira "Cima" (Anterior)
            ESP_LOGI(TAG, "Nível 2: Traduzindo LEFT para PREV");
            lv_group_focus_prev(g_main);
        } else if(key == LV_KEY_RIGHT) {
            // Tradução dinâmica: No Nível 2, Direita vira "Baixo" (Próximo)
            ESP_LOGI(TAG, "Nível 2: Traduzindo RIGHT para NEXT");
            lv_group_focus_next(g_main);
        }
    }
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

    ESP_LOGI(TAG, "--- INTEGRAÇÃO DE NAVEGAÇÃO POR BOTÕES (KEYPAD) ---");
    hal_buttons_init();
    lv_indev_t * indev = hal_buttons_get_indev();

    g_main = lv_group_create();
    // REMOVIDO: lv_group_set_default(g); para evitar que tudo entre no grupo automaticamente
    lv_indev_set_group(indev, g_main);

    ESP_LOGI(TAG, "Construindo Navegação...");
    
    /* TabView em baixo */
    tv = lv_tabview_create(lv_scr_act(), LV_DIR_BOTTOM, 50);
    lv_group_add_obj(g_main, tv); // O grupo começa controlando apenas as abas (Nível 1)
    lv_group_focus_obj(tv);      // Garante que o TabView comece com o foco
    lv_obj_add_event_cb(tv, tabview_event_cb, LV_EVENT_KEY, NULL);
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

    create_screen_readings(t1);
    create_screen_connectivity(t2);
    create_screen_device(t3);
    create_screen_info(t4);

    ESP_LOGI(TAG, "Interface pronta.");
}
