#ifndef HAL_DISPLAY_H
#define HAL_DISPLAY_H

#include "esp_lcd_types.h"

/**
 * @brief Inicializa o display LCD e o Touch (SST7789 e I2C Touch).
 *        Prepara a camada inferior para conectar com a LVGL.
 */
void hal_display_init(void);

/**
 * @brief Retorna o handle do painel LCD configurado.
 */
esp_lcd_panel_handle_t hal_display_get_panel_handle(void);

/**
 * @brief Retorna o handle do barramento de IO (SPI/DMA) para sincronismo de V-Sync.
 */
esp_lcd_panel_io_handle_t hal_display_get_io_handle(void);

#endif
