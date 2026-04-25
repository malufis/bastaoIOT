#include "hal_display.h"
#include "esp_log.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_ili9341.h"
#include "driver/i2c.h"

static const char *TAG = "HAL_DISPLAY";

static esp_lcd_panel_handle_t panel_handle = NULL;
static esp_lcd_panel_io_handle_t static_io_handle = NULL;

// Pinagem Unihiker K10 (ESP32-S3)
#define I2C_MASTER_SDA 47
#define I2C_MASTER_SCL 48
#define XL9535_ADDR    0x20 // Endereço oficial do K10 (Tente 0x24 se falhar)

#define LCD_HOST       SPI2_HOST
#define PIN_NUM_SCLK   12
#define PIN_NUM_MOSI   21
#define PIN_NUM_MISO   41
#define PIN_NUM_CS     14  // Pino CS oficial do K10
#define PIN_NUM_DC     13
#define PIN_NUM_RST    -1  // Reset padrão do hardware
#define PIN_NUM_BK_LIGHT -1 // Controlado via Expansor I2C

#define LCD_H_RES      240
#define LCD_V_RES      320


/**
 * @brief Envia comando I2C para o expansor XL9535 com log de erro
 */
static esp_err_t xl9535_write_reg(uint8_t reg, uint8_t data) {
    uint8_t write_buf[2] = {reg, data};
    esp_err_t err = i2c_master_write_to_device(I2C_NUM_0, XL9535_ADDR, write_buf, sizeof(write_buf), pdMS_TO_TICKS(100));
    if (err != ESP_OK) {
        ESP_LOGE("XL9535", "Falha na comunicacao I2C (Reg 0x%02X): %s", reg, esp_err_to_name(err));
    }
    return err;
}

void hal_display_init(void) {
    // 1. Inicializa I2C para falar com o Expansor
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA,
        .scl_io_num = I2C_MASTER_SCL,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 100000,
    };
    i2c_param_config(I2C_NUM_0, &conf);
    i2c_driver_install(I2C_NUM_0, conf.mode, 0, 0, 0);

    ESP_LOGI(TAG, "Iniciando Scanner I2C:");
    for (uint8_t i = 1; i < 127; i++) {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (i << 1) | I2C_MASTER_WRITE, true);
        i2c_master_stop(cmd);
        esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, pdMS_TO_TICKS(50));
        i2c_cmd_link_delete(cmd);
        if (ret == ESP_OK) {
            ESP_LOGW(TAG, "=> Dispositivo I2C ENCONTRADO no endereco: 0x%02X", i);
        }
    }
    ESP_LOGI(TAG, "Scanner I2C finalizado.");

    // Configura XL9535: P00 (Backlight do LCD) como Saída (Reg 0x06 é Config Port 0)
    // 0xFE = 11111110 (P00 como 0 -> Saída)
    xl9535_write_reg(0x06, 0xFE); 
    // Seta P00 em HIGH para ligar o Backlight (Reg 0x02 é Output Port 0)
    // 0x01 = 00000001
    xl9535_write_reg(0x02, 0x01);

    ESP_LOGI(TAG, "Backlight ativado via XL9535. Inicializando SPI...");
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_NUM_SCLK,
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * 80 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Configurando IO do Painel LCD...");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_DC,
        .cs_gpio_num = PIN_NUM_CS,
        .pclk_hz = 10 * 1000 * 1000, // Reduzido para 10MHz p/ estabilidade absoluta
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));
    static_io_handle = io_handle; // Salva para o sincronismo com LVGL

    ESP_LOGI(TAG, "Instalando driver ILI9341...");
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR, // BGR para cores corretas (junto com Menuconfig)
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_ili9341(io_handle, &panel_config, &panel_handle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    
    /* Configuração exata para o vidro do K10 */
    esp_lcd_panel_invert_color(panel_handle, false); // Desligado para fundo Dark
    esp_lcd_panel_mirror(panel_handle, false, true); // <-- Sem espelhamento em X ou Y
    esp_lcd_panel_swap_xy(panel_handle, false);      // <-- Modo Retrato Nativo (Não inverte largura com altura)
    
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
}

esp_lcd_panel_handle_t hal_display_get_panel_handle(void) {
    return panel_handle;
}

esp_lcd_panel_io_handle_t hal_display_get_io_handle(void) {
    return static_io_handle;
}
