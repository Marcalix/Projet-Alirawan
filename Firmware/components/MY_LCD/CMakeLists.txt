idf_component_register(SRCS "MY_LCD.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "lvgl" "esp_lvgl_port" "esp_lcd_touch_ft5x06" "esp_lcd_st7796" "driver" "esp_timer")

