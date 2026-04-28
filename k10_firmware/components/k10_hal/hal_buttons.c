#include "hal_buttons.h"
#include "driver/i2c.h"
#include "esp_log.h"

#define I2C_PORT_NUM    I2C_NUM_0
#define XL9535_ADDR     0x20

static const char *TAG = "HAL_BUTTONS";
static lv_indev_t *indev_keypad = NULL;

static uint8_t xl9535_read_reg(uint8_t reg) {
    uint8_t data = 0;
    esp_err_t err = i2c_master_write_read_device(I2C_PORT_NUM, XL9535_ADDR, &reg, 1, &data, 1, pdMS_TO_TICKS(10));
    if (err != ESP_OK) {
        return 0xFF; // Assume disconnected = pull-ups high
    }
    return data;
}

static uint32_t keypad_get_key(void) {
    uint8_t port0 = xl9535_read_reg(0);
    uint8_t port1 = xl9535_read_reg(1);

    bool btn_a = !(port1 & (1 << 4)); // Key A -> P5 -> Port 1, Bit 4
    bool btn_b = !(port0 & (1 << 2)); // Key B -> P11 -> Port 0, Bit 2
    
    if (btn_a) return LV_KEY_LEFT; 
    if (btn_b) return LV_KEY_RIGHT; 
    return 0; 
}

static void keypad_read(lv_indev_drv_t * indev_drv, lv_indev_data_t * data) {
    static uint32_t last_raw_key = 0;
    static uint32_t press_start_time = 0;
    static bool long_press_active = false;
    static bool short_press_pending = false;

    uint32_t act_key = keypad_get_key();
    uint32_t current_time = lv_tick_get();

    data->continue_reading = false;

    if(act_key != 0) {
        if(last_raw_key == 0) {
            // Início de um novo pressionamento
            last_raw_key = act_key;
            press_start_time = current_time;
            long_press_active = false;
            short_press_pending = true;
        }

        // Verifica se atingiu 3 segundos para toque longo
        if(!long_press_active && (current_time - press_start_time >= 3000)) {
            long_press_active = true;
            short_press_pending = false; // Cancela a intenção de toque curto
            data->key = (last_raw_key == LV_KEY_LEFT) ? LV_KEY_ENTER : LV_KEY_ESC;
            data->state = LV_INDEV_STATE_PR;
            ESP_LOGI(TAG, "Long Press Detectado: %s", (data->key == LV_KEY_ENTER) ? "ENTER" : "ESC");
        } else if(long_press_active) {
            // Mantém o estado de pressionado para o toque longo
            data->key = (last_raw_key == LV_KEY_LEFT) ? LV_KEY_ENTER : LV_KEY_ESC;
            data->state = LV_INDEV_STATE_PR;
        } else {
            // Botão pressionado mas ainda não deu 3s (aguardando para decidir se é curto ou longo)
            data->state = LV_INDEV_STATE_REL;
        }
    } else {
        // Botão foi solto
        if(last_raw_key != 0) {
            if(short_press_pending) {
                // Era um toque curto! Envia PR agora e sinaliza para enviar REL logo em seguida
                data->key = last_raw_key;
                data->state = LV_INDEV_STATE_PR;
                short_press_pending = false;
                data->continue_reading = true; 
                ESP_LOGI(TAG, "Short Press Detectado: %d", (int)data->key);
            } else {
                // Liberação de um toque longo ou ciclo de curto finalizado
                data->key = (long_press_active) ? ((last_raw_key == LV_KEY_LEFT) ? LV_KEY_ENTER : LV_KEY_ESC) : last_raw_key;
                data->state = LV_INDEV_STATE_REL;
                last_raw_key = 0;
                long_press_active = false;
            }
        } else {
            data->state = LV_INDEV_STATE_REL;
        }
    }
}

void hal_buttons_init(void) {
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_KEYPAD;
    indev_drv.read_cb = keypad_read;

    indev_keypad = lv_indev_drv_register(&indev_drv);
    ESP_LOGI(TAG, "Driver de Botoes iniciado. A=LEFT (A+3s=ENTER), B=RIGHT (B+3s=ESC)");
}

lv_indev_t* hal_buttons_get_indev(void) {
    return indev_keypad;
}
