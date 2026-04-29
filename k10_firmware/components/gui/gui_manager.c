#include "gui_manager.h"
#include "gui_theme.h"
#include "esp_log.h"
#include "lvgl.h"
#include "hal_display.h"
#include "hal_buttons.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_io.h"
#include "hal_sensors.h"
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
static bool is_internal_focus = false;

/* Labels Dinâmicas para Sensores */
static lv_obj_t *lbl_bat_pct;
static lv_obj_t *icon_bat_header;
static lv_obj_t *lbl_telemetry;
static lv_obj_t *lbl_gps_coords;

static void enter_screen_focus(void) {
    if(is_internal_focus) return;

    uint16_t act_tab = lv_tabview_get_tab_act(tv);
    if(act_tab == 0) {
        ESP_LOGI(TAG, "Tela Home nao possui navegacao interna. Ignorando.");
        return;
    }

    lv_obj_t * content = lv_tabview_get_content(tv);
    lv_obj_t * act_tab_obj = lv_obj_get_child(content, act_tab);
    
    if(!act_tab_obj) return;

    lv_obj_t * target_obj = (lv_obj_t *)lv_obj_get_user_data(act_tab_obj);
    ESP_LOGI(TAG, "Tentando entrar no foco da aba %d. Target: %p", act_tab, target_obj);

    if(target_obj) {
        is_internal_focus = true;
        lv_group_remove_all_objs(g_main);
        
        uint32_t child_cnt = lv_obj_get_child_cnt(target_obj);
        uint32_t added_cnt = 0;

        for(uint32_t i = 0; i < child_cnt; i++) {
            lv_obj_t * child = lv_obj_get_child(target_obj, i);
            // Adiciona apenas se for clicável/focável (botões da lista)
            if(lv_obj_has_flag(child, LV_OBJ_FLAG_CLICKABLE)) {
                lv_group_add_obj(g_main, child);
                added_cnt++;
            }
        }
        
        // Força o scroll para o topo absoluto (0) para garantir que o primeiro Header apareça
        lv_obj_scroll_to_y(target_obj, 0, LV_ANIM_OFF);
        
        ESP_LOGI(TAG, "Foco interno ativado. Itens no grupo: %d", (int)added_cnt);
    } else {
        ESP_LOGW(TAG, "Nenhum objeto de foco definido para a aba %d.", act_tab);
    }
}

static void exit_screen_focus(void) {
    if(!is_internal_focus) return;
    
    is_internal_focus = false;
    lv_group_remove_all_objs(g_main);
    lv_group_add_obj(g_main, tv);
    lv_group_focus_obj(tv);
    ESP_LOGI(TAG, "Retornando ao Nível 1 (Abas)");
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
    lv_obj_set_style_pad_all(parent, 0, 0);
    lv_obj_set_style_pad_row(parent, 0, 0);

    // --- LINHA SUPERIOR (Header - Proporção 1) ---
    lv_obj_t * header = lv_obj_create(parent);
    lv_obj_set_size(header, lv_pct(100), 34);
    lv_obj_set_style_bg_opa(header, 0, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_pad_all(header, 5, 0);
    
    // Ícones Superior Direito (4G -> WiFi -> RFID -> GPS)
    lv_obj_t * icons_cont = lv_obj_create(header);
    lv_obj_set_size(icons_cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(icons_cont, 0, 0);
    lv_obj_set_style_border_width(icons_cont, 0, 0);
    lv_obj_align(icons_cont, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_flex_flow(icons_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(icons_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(icons_cont, 8, 0);

    lv_obj_t * icon_4g = lv_label_create(icons_cont);
    lv_label_set_text(icon_4g, "4G");
    lv_obj_set_style_text_color(icon_4g, COLOR_EMERALD, 0);

    lv_obj_t * icon_wifi = lv_label_create(icons_cont);
    lv_label_set_text(icon_wifi, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(icon_wifi, COLOR_EMERALD, 0);

    lv_obj_t * icon_rfid = lv_label_create(icons_cont);
    lv_label_set_text(icon_rfid, "RFID"); // UHF ou LF/RF
    lv_obj_set_style_text_color(icon_rfid, COLOR_EMERALD, 0);

    lv_obj_t * icon_gps = lv_label_create(icons_cont);
    lv_label_set_text(icon_gps, LV_SYMBOL_GPS);
    lv_obj_set_style_text_color(icon_gps, COLOR_EMERALD, 0);

    lv_obj_t * bat_cont = lv_obj_create(header);
    lv_obj_set_size(bat_cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(bat_cont, 0, 0);
    lv_obj_set_style_border_width(bat_cont, 0, 0);
    lv_obj_align(bat_cont, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_flex_flow(bat_cont, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bat_cont, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(bat_cont, 4, 0);

    icon_bat_header = lv_label_create(bat_cont);
    lv_label_set_text(icon_bat_header, LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_text_color(icon_bat_header, COLOR_EMERALD, 0);

    lbl_bat_pct = lv_label_create(bat_cont);
    lv_label_set_text(lbl_bat_pct, "75%");
    lv_obj_set_style_text_color(lbl_bat_pct, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl_bat_pct, &lv_font_montserrat_14, 0);

    // --- ÁREA CENTRAL (Dados - Proporção 5) ---
    lv_obj_t * center = lv_obj_create(parent);
    lv_obj_set_size(center, lv_pct(95), 168);
    lv_obj_add_style(center, &style_card, 0);
    lv_obj_set_style_pad_all(center, 10, 0);
    lv_obj_set_flex_flow(center, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(center, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    lv_obj_t * farm_title = lv_label_create(center);
    lv_label_set_text(farm_title, "Fazenda Raptor");
    lv_obj_set_style_text_color(farm_title, COLOR_GRAY_TEXT, 0);

    lv_obj_t * bovine_id = lv_label_create(center);
    lv_label_set_text(bovine_id, "Boi Bandido");
    lv_obj_set_style_text_font(bovine_id, &lv_font_montserrat_16, 0); 
    lv_obj_set_style_text_color(bovine_id, lv_color_white(), 0);

    lv_obj_t * time_label = lv_label_create(center);
    lv_label_set_text(time_label, "25/04/2026 as 07:15");
    lv_obj_set_style_text_color(time_label, lv_color_white(), 0);

    lbl_gps_coords = lv_label_create(center);
    lv_label_set_text(lbl_gps_coords, "-20.4442036710, -54.6194435308");
    lv_obj_set_style_text_color(lbl_gps_coords, COLOR_GRAY_TEXT, 0);
    lv_obj_set_style_text_align(lbl_gps_coords, LV_TEXT_ALIGN_CENTER, 0);

    // --- LINHA INFERIOR (Footer - Proporção 2) ---
    lv_obj_t * footer = lv_obj_create(parent);
    lv_obj_set_size(footer, lv_pct(100), 68);
    lv_obj_set_style_bg_opa(footer, 0, 0);
    lv_obj_set_style_border_width(footer, 0, 0);
    lv_obj_set_flex_flow(footer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(footer, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lbl_telemetry = lv_label_create(footer);
    lv_label_set_text(lbl_telemetry, "Inc: X:0.0 Y:0.0 Z:0.0");
    lv_obj_set_style_text_color(lbl_telemetry, COLOR_GRAY_TEXT, 0);
    lv_obj_set_style_text_align(lbl_telemetry, LV_TEXT_ALIGN_CENTER, 0);

    // Sem foco interno para a Tela Home
    lv_obj_set_user_data(parent, NULL);
}

static void create_screen_connectivity(lv_obj_t * parent) {
    lv_obj_set_style_bg_color(parent, COLOR_BG_DARK, 0);
    lv_obj_t * list = lv_list_create(parent);
    lv_obj_set_size(list, lv_pct(100), lv_pct(95));
    lv_obj_align(list, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(list, COLOR_BG_DARK, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    
    lv_obj_t * btn;
    lv_obj_t * txt;

    // --- SECAO 4G/3G ---
    txt = lv_list_add_text(list, "REDES MOVEIS (4G/3G)");
    lv_obj_set_style_bg_color(txt, COLOR_ANTHRACITE, 0);
    lv_obj_set_style_bg_opa(txt, 255, 0);
    lv_obj_set_style_text_color(txt, COLOR_WHITE, 0);

    btn = lv_list_add_btn(list, LV_SYMBOL_SD_CARD, "APN: vivo.com.br");
    lv_obj_add_event_cb(btn, internal_obj_event_cb, LV_EVENT_KEY, NULL);
    btn = lv_list_add_btn(list, LV_SYMBOL_DIRECTORY, "Operadora: VIVO");
    lv_obj_add_event_cb(btn, internal_obj_event_cb, LV_EVENT_KEY, NULL);

    // --- SECAO WIFI ---
    txt = lv_list_add_text(list, "CONEXAO WIFI");
    lv_obj_set_style_bg_color(txt, COLOR_ANTHRACITE, 0);
    lv_obj_set_style_bg_opa(txt, 255, 0);
    lv_obj_set_style_text_color(txt, COLOR_WHITE, 0);

    btn = lv_list_add_btn(list, LV_SYMBOL_WIFI, "SSID: Fazenda_Raptor");
    lv_obj_add_event_cb(btn, internal_obj_event_cb, LV_EVENT_KEY, NULL);
    btn = lv_list_add_btn(list, LV_SYMBOL_OK, "Estado: Conectado");
    lv_obj_add_event_cb(btn, internal_obj_event_cb, LV_EVENT_KEY, NULL);

    // --- SECAO RFID ---
    txt = lv_list_add_text(list, "MODULO RFID");
    lv_obj_set_style_bg_color(txt, COLOR_ANTHRACITE, 0);
    lv_obj_set_style_bg_opa(txt, 255, 0);
    lv_obj_set_style_text_color(txt, COLOR_WHITE, 0);

    btn = lv_list_add_btn(list, LV_SYMBOL_REFRESH, "Modo: UHF");
    lv_obj_add_event_cb(btn, internal_obj_event_cb, LV_EVENT_KEY, NULL);
    btn = lv_list_add_btn(list, LV_SYMBOL_CHARGE, "Potencia: 30dBm");
    lv_obj_add_event_cb(btn, internal_obj_event_cb, LV_EVENT_KEY, NULL);

    // --- SECAO GPS ---
    txt = lv_list_add_text(list, "LOCALIZACAO (GPS/GLONASS)");
    lv_obj_set_style_bg_color(txt, COLOR_ANTHRACITE, 0);
    lv_obj_set_style_bg_opa(txt, 255, 0);
    lv_obj_set_style_text_color(txt, COLOR_WHITE, 0);

    btn = lv_list_add_btn(list, LV_SYMBOL_GPS, "Ativar: GPS + GLONASS");
    lv_obj_add_event_cb(btn, internal_obj_event_cb, LV_EVENT_KEY, NULL);

    lv_obj_set_user_data(parent, list); 
}

static void create_screen_device(lv_obj_t * parent) {
    lv_obj_set_style_bg_color(parent, COLOR_BG_DARK, 0);
    lv_obj_t * list = lv_list_create(parent);
    lv_obj_set_size(list, lv_pct(100), lv_pct(95));
    lv_obj_align(list, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(list, COLOR_BG_DARK, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    
    lv_obj_t * btn;
    lv_obj_t * txt;

    // --- SECAO APP ---
    txt = lv_list_add_text(list, "CONFIGURACAO DO APP");
    lv_obj_set_style_bg_color(txt, COLOR_ANTHRACITE, 0);
    lv_obj_set_style_bg_opa(txt, 255, 0);
    lv_obj_set_style_text_color(txt, COLOR_WHITE, 0);

    btn = lv_list_add_btn(list, LV_SYMBOL_REFRESH, "Sincronizacao: Automatica");
    lv_obj_add_event_cb(btn, internal_obj_event_cb, LV_EVENT_KEY, NULL);
    btn = lv_list_add_btn(list, LV_SYMBOL_OK, "Ativacao de Dispositivo");
    lv_obj_add_event_cb(btn, internal_obj_event_cb, LV_EVENT_KEY, NULL);

    // --- SECAO ARQUIVOS ---
    txt = lv_list_add_text(list, "GERENCIAMENTO DE MEMORIA");
    lv_obj_set_style_bg_color(txt, COLOR_ANTHRACITE, 0);
    lv_obj_set_style_bg_opa(txt, 255, 0);
    lv_obj_set_style_text_color(txt, COLOR_WHITE, 0);

    btn = lv_list_add_btn(list, LV_SYMBOL_FILE, "Arquivos: 142");
    lv_obj_add_event_cb(btn, internal_obj_event_cb, LV_EVENT_KEY, NULL);
    btn = lv_list_add_btn(list, LV_SYMBOL_DRIVE, "Memoria Livre: 850MB");
    lv_obj_add_event_cb(btn, internal_obj_event_cb, LV_EVENT_KEY, NULL);

    // --- SECAO ENVIO ---
    txt = lv_list_add_text(list, "TRANSFERENCIA DE DADOS");
    lv_obj_set_style_bg_color(txt, COLOR_ANTHRACITE, 0);
    lv_obj_set_style_bg_opa(txt, 255, 0);
    lv_obj_set_style_text_color(txt, COLOR_WHITE, 0);

    btn = lv_list_add_btn(list, LV_SYMBOL_UPLOAD, "Enviar arquivo por demanda");
    lv_obj_add_event_cb(btn, internal_obj_event_cb, LV_EVENT_KEY, NULL);

    // --- SECAO SERVIDOR ---
    txt = lv_list_add_text(list, "INFRAESTRUTURA");
    lv_obj_set_style_bg_color(txt, COLOR_ANTHRACITE, 0);
    lv_obj_set_style_bg_opa(txt, 255, 0);
    lv_obj_set_style_text_color(txt, COLOR_WHITE, 0);

    btn = lv_list_add_btn(list, LV_SYMBOL_UPLOAD, "Servidor: Online");
    lv_obj_add_event_cb(btn, internal_obj_event_cb, LV_EVENT_KEY, NULL);

    lv_obj_set_user_data(parent, list); 
}

static void create_screen_info(lv_obj_t * parent) {
    lv_obj_set_style_bg_color(parent, COLOR_BG_DARK, 0);
    lv_obj_t * list = lv_list_create(parent);
    lv_obj_set_size(list, lv_pct(100), lv_pct(95));
    lv_obj_align(list, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(list, COLOR_BG_DARK, 0);
    lv_obj_set_style_border_width(list, 0, 0);
    
    lv_obj_t * btn;
    lv_obj_t * txt;

    txt = lv_list_add_text(list, "DIAGNOSTICO DO SISTEMA");
    lv_obj_set_style_bg_color(txt, COLOR_ANTHRACITE, 0);
    lv_obj_set_style_bg_opa(txt, 255, 0);
    lv_obj_set_style_text_color(txt, COLOR_WHITE, 0);
    
    btn = lv_list_add_btn(list, LV_SYMBOL_PLAY, "Firmware: v2.1.0-bastao");
    lv_obj_add_event_cb(btn, internal_obj_event_cb, LV_EVENT_KEY, NULL);

    btn = lv_list_add_btn(list, LV_SYMBOL_GPS, "Satelites: 12");
    lv_obj_add_event_cb(btn, internal_obj_event_cb, LV_EVENT_KEY, NULL);

    btn = lv_list_add_btn(list, LV_SYMBOL_CHARGE, "Sinal GPS/GLONASS: -130dBm");
    lv_obj_add_event_cb(btn, internal_obj_event_cb, LV_EVENT_KEY, NULL);

    btn = lv_list_add_btn(list, LV_SYMBOL_SD_CARD, "Sinal 4G/GSM: -85dBm");
    lv_obj_add_event_cb(btn, internal_obj_event_cb, LV_EVENT_KEY, NULL);

    btn = lv_list_add_btn(list, LV_SYMBOL_BLUETOOTH, "Potencia UHF: Alta");
    lv_obj_add_event_cb(btn, internal_obj_event_cb, LV_EVENT_KEY, NULL);

    btn = lv_list_add_btn(list, LV_SYMBOL_USB, "Cliente: ESP32_SENSOR_01");
    lv_obj_add_event_cb(btn, internal_obj_event_cb, LV_EVENT_KEY, NULL);

    lv_obj_set_user_data(parent, list); 
}

static void tabview_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    uint32_t key = lv_event_get_key(e);

    if(code == LV_EVENT_KEY) {
        if(is_internal_focus) return; // Se estiver no nível 2, ignora eventos de aba

        ESP_LOGI(TAG, "Tecla recebida no TabView: %d", (int)key);
        if(key == LV_KEY_LEFT) {
            uint16_t tab = lv_tabview_get_tab_act(tv);
            if(tab < 3) lv_tabview_set_act(tv, tab + 1, LV_ANIM_ON);
        } else if(key == LV_KEY_RIGHT) {
            uint16_t tab = lv_tabview_get_tab_act(tv);
            if(tab > 0) lv_tabview_set_act(tv, tab - 1, LV_ANIM_ON);
        } else if(key == LV_KEY_ENTER) {
            ESP_LOGI(TAG, "Botão A 3s detectado: Tentando entrar na tela");
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
            lv_obj_scroll_to_view(lv_group_get_focused(g_main), LV_ANIM_ON);
        } else if(key == LV_KEY_RIGHT) {
            // Tradução dinâmica: No Nível 2, Direita vira "Baixo" (Próximo)
            ESP_LOGI(TAG, "Nível 2: Traduzindo RIGHT para NEXT");
            lv_group_focus_next(g_main);
            lv_obj_scroll_to_view(lv_group_get_focused(g_main), LV_ANIM_ON);
        } else if(key == LV_KEY_ENTER) {
            ESP_LOGI(TAG, "Nível 2: Botão A 3s - Selecionando!");
            lv_event_send(lv_event_get_target(e), LV_EVENT_CLICKED, NULL);
        }
    }
}

void gui_manager_init(void) {
    ESP_LOGI(TAG, "Inicializando LVGL...");
    lv_init();

    esp_lcd_panel_handle_t panel_handle = hal_display_get_panel_handle();

    static lv_disp_draw_buf_t draw_buf;
    static lv_color_t buf[240 * 100]; // Aumentado para 100 linhas para máxima performance
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, 240 * 100);

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

void gui_manager_update_sensors(accel_data_t *accel, battery_data_t *bat) {
    char buf[64];

    if(bat) {
        snprintf(buf, sizeof(buf), "%d%%", bat->percentage);
        lv_label_set_text(lbl_bat_pct, buf);
        
        // Icone dinâmico e cor
        if(bat->percentage > 80) lv_label_set_text(icon_bat_header, LV_SYMBOL_BATTERY_FULL);
        else if(bat->percentage > 50) lv_label_set_text(icon_bat_header, LV_SYMBOL_BATTERY_3);
        else if(bat->percentage > 20) lv_label_set_text(icon_bat_header, LV_SYMBOL_BATTERY_2);
        else lv_label_set_text(icon_bat_header, LV_SYMBOL_BATTERY_1);

        if(bat->percentage < 20) {
            lv_obj_set_style_text_color(lbl_bat_pct, lv_palette_main(LV_PALETTE_RED), 0);
            lv_obj_set_style_text_color(icon_bat_header, lv_palette_main(LV_PALETTE_RED), 0);
        } else {
            lv_obj_set_style_text_color(lbl_bat_pct, lv_color_white(), 0);
            lv_obj_set_style_text_color(icon_bat_header, COLOR_EMERALD, 0);
        }
    }

    if(accel) {
        snprintf(buf, sizeof(buf), "Inc: X:%.1f Y:%.1f Z:%.1f", accel->x, accel->y, accel->z);
        lv_label_set_text(lbl_telemetry, buf);
        ESP_LOGD(TAG, "GUI Update: Accel Data applied");
    }
}
