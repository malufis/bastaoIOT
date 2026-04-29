#ifndef HAL_SENSORS_H
#define HAL_SENSORS_H

#include "esp_err.h"

/**
 * @brief Estrutura para dados do acelerômetro
 */
typedef struct {
    float x;
    float y;
    float z;
} accel_data_t;

/**
 * @brief Estrutura para dados da bateria
 */
typedef struct {
    float voltage;
    int percentage;
} battery_data_t;

/**
 * @brief Inicializa os sensores do Unihiker K10 (Acelerômetro e Bateria)
 */
esp_err_t hal_sensors_init(void);

/**
 * @brief Lê os dados brutos do acelerômetro SC7A20H e converte para G
 */
esp_err_t hal_sensors_read_accel(accel_data_t *data);

/**
 * @brief Lê a tensão da bateria via ADC e calcula a porcentagem
 */
esp_err_t hal_sensors_read_battery(battery_data_t *data);

#endif
