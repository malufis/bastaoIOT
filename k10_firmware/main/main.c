#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "hal_display.h"
#include "gui_manager.h"

#include "lvgl.h"

static const char *TAG = "K10_MAIN";

/**
 * @brief Thread dedicada à Interface Gráfica LVGL (Fixada no Core 1).
 */
void gui_task(void *pvParameters) {
    ESP_LOGI(TAG, "Iniciando Task LVGL");
    
    // 1. Inicializa driver LCD e Touch
    hal_display_init();

    // 2. Inicializa LVGL e Gerenciador de UI
    gui_manager_init();

    while (1) {
        // O lv_timer_handler precisa ser chamado periodicamente
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
        // Informa o LVGL que se passaram 10ms
        lv_tick_inc(10);
    }
}

/**
 * @brief Entry point do firmware K10.
 */
void app_main(void) {
    ESP_LOGI(TAG, "Inicializando Bastão Unihiker K10...");

    // Inicializa NVS (Non-Volatile Storage) para configs de rede
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Cria a Task da Interface Gráfica fixada no APP_CPU (Core 1)
    xTaskCreatePinnedToCore(gui_task, "gui_task", 1024 * 8, NULL, 5, NULL, 1);

    // TODO: Criar a Task do Gateway/Rede fixada no PRO_CPU (Core 0)
}
