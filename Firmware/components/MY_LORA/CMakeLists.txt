idf_component_register(SRCS "MY_LORA.c" "GLOBAL_REQUEST.c" "LoRaWAN_REQUEST.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "driver"  "esp_timer")

