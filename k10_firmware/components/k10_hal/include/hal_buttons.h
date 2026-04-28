#ifndef HAL_BUTTONS_H
#define HAL_BUTTONS_H

#include "lvgl.h"

/**
 * @brief Inicializa e vincula os botões físicos (I2C XL9535) ao LVGL (Keypad Indev).
 */
void hal_buttons_init(void);

/**
 * @brief Retorna o dispositivo de entrada (indev) para ser associado a um lv_group.
 */
lv_indev_t* hal_buttons_get_indev(void);

#endif
