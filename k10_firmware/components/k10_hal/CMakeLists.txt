idf_component_register(SRCS "hal_display.c" "hal_buttons.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_lcd esp_lcd_ili9341 esp_timer driver lvgl__lvgl)
