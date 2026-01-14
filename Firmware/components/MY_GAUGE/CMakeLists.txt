idf_component_register(SRCS "MY_GAUGE.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "driver" "esp_timer" "max17048" "i2c_bus")