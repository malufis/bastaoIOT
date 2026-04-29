#ifndef GUI_MANAGER_H
#define GUI_MANAGER_H

#include "hal_sensors.h"

/**
 * @brief Inicializa a engine da interface gráfica LVGL e renderiza a tela Inicial.
 */
void gui_manager_init(void);

/**
 * @brief Atualiza os valores dos sensores na interface gráfica.
 */
void gui_manager_update_sensors(accel_data_t *accel, battery_data_t *bat);

#endif
