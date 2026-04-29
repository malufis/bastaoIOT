idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES freertos nvs_flash gui k10_hal k10_sensors)
