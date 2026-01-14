idf_component_register(SRCS "MY_GNSS.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "driver"  "esp_timer"  "MINMEA" )