idf_component_register(SRCS "MY_RTC.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "driver"  "esp_timer" "i2c_bus" "ds3231")