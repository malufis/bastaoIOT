#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "hal_display.h"
#include "gui_manager.h"
#include "hal_sensors.h"

#include "lvgl.h"

static const char *TAG = "K10_MAIN";

/**
 * @brief Thread dedicada à Interface Gráfica LVGL (Fixada no Core 1).
 */
void gui_task(void *pvParameters) {
    ESP_LOGI(TAG, "Iniciando Task LVGL e Sensores");
    
    // 1. Inicializa driver LCD, Touch e Sensores
    hal_display_init();
    hal_sensors_init();

    // 2. Inicializa LVGL e Gerenciador de UI
    gui_manager_init();

    uint32_t last_tick = esp_log_timestamp();
    int sensor_timer = 0;

    while (1) {
        uint32_t current_tick = esp_log_timestamp();
        lv_tick_inc(current_tick - last_tick);
        last_tick = current_tick;

        // Atualiza sensores a cada 500ms
        if(sensor_timer++ >= 50) { 
            accel_data_t accel;
            battery_data_t bat;
            if(hal_sensors_read_accel(&accel) == ESP_OK && hal_sensors_read_battery(&bat) == ESP_OK) {
                gui_manager_update_sensors(&accel, &bat);
            }
            sensor_timer = 0;
        }

        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
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
