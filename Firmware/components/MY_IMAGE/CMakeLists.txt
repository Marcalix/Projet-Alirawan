idf_component_register(SRCS "MY_IMAGE.c" "Load_img.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "driver" "esp_timer" "esp_lvgl_port" "lvgl")