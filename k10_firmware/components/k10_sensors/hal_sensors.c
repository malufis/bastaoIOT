#include "hal_sensors.h"
#include "driver/i2c.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_log.h"
#include <string.h>

#define I2C_PORT_NUM    I2C_NUM_0
#define SC7A20_ADDR     0x19 

// Monitoramento de Bateria - Detectado CH7 (GPIO 8) como estável em ~2620 (Full Battery)
#define BAT_ADC_UNIT     ADC_UNIT_1
#define BAT_ADC_CHANNEL  ADC_CHANNEL_7 
#define BAT_ADC_ATTEN    ADC_ATTEN_DB_12

static const char *TAG = "HAL_SENSORS";

static adc_oneshot_unit_handle_t adc1_handle;
static adc_cali_handle_t adc1_cali_handle = NULL;
static bool do_calibration = false;

esp_err_t hal_sensors_init(void) {
    uint8_t ctrl1[2] = {0x20, 0x27}; 
    i2c_master_write_to_device(I2C_PORT_NUM, SC7A20_ADDR, ctrl1, 2, pdMS_TO_TICKS(100));
    uint8_t ctrl4[2] = {0x23, 0x80}; 
    i2c_master_write_to_device(I2C_PORT_NUM, SC7A20_ADDR, ctrl4, 2, pdMS_TO_TICKS(100));

    adc_oneshot_unit_init_cfg_t init_config1 = { .unit_id = BAT_ADC_UNIT };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));
    
    adc_oneshot_chan_cfg_t config = { .bitwidth = ADC_BITWIDTH_DEFAULT, .atten = BAT_ADC_ATTEN };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, BAT_ADC_CHANNEL, &config));

    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = BAT_ADC_UNIT, .chan = BAT_ADC_CHANNEL, .atten = BAT_ADC_ATTEN, .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    if (adc_cali_create_scheme_curve_fitting(&cali_config, &adc1_cali_handle) == ESP_OK) do_calibration = true;

    return ESP_OK;
}

esp_err_t hal_sensors_read_accel(accel_data_t *data) {
    if (!data) return ESP_ERR_INVALID_ARG;
    uint8_t reg = 0x28 | 0x80; 
    uint8_t raw[6];
    esp_err_t err = i2c_master_write_read_device(I2C_PORT_NUM, SC7A20_ADDR, &reg, 1, raw, 6, pdMS_TO_TICKS(100));
    if (err != ESP_OK) return err;
    int16_t ix = (int16_t)(raw[0] | (raw[1] << 8));
    int16_t iy = (int16_t)(raw[2] | (raw[3] << 8));
    int16_t iz = (int16_t)(raw[4] | (raw[5] << 8));
    data->x = (float)ix / 16384.0f;
    data->y = (float)iy / 16384.0f;
    data->z = (float)iz / 16384.0f;
    return ESP_OK;
}

esp_err_t hal_sensors_read_battery(battery_data_t *data) {
    if (!data) return ESP_ERR_INVALID_ARG;
    int adc_raw = 0;
    int voltage_mv = 0;
    
    for(int i=0; i<16; i++) {
        int val;
        adc_oneshot_read(adc1_handle, BAT_ADC_CHANNEL, &val);
        adc_raw += val;
    }
    adc_raw /= 16;

    if (do_calibration) adc_cali_raw_to_voltage(adc1_cali_handle, adc_raw, &voltage_mv);
    else voltage_mv = (adc_raw * 3300) / 4095;

    // Divisor 1:2 (Padrão para CH7/GPIO 8 no Unihiker K10)
    // 2620 raw ~ 2110mV. 2110mV * 2 = 4220mV (4.22V - Full)
    data->voltage = (float)voltage_mv * 2.0f / 1000.0f;

    // Escala 3.4V (0%) a 4.2V (100%)
    float pct = (data->voltage - 3.4f) / (4.2f - 3.4f) * 100.0f;
    if (pct > 100.0f) pct = 100.0f; else if (pct < 0.0f) pct = 0.0f;
    data->percentage = (int)pct;

    ESP_LOGD(TAG, "BAT: Raw=%d, V=%.2f, Pct=%d%%", adc_raw, data->voltage, data->percentage);
    return ESP_OK;
}
