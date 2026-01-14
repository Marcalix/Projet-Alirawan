idf_component_register(SRCS "MY_WINDOWS.c" "LOAD_WINDOWS.c" "HOME_WINDOWS.c"
                    INCLUDE_DIRS "include"
                    REQUIRES "MY_IMAGE" "driver" "esp_timer" "esp_lvgl_port" "lvgl")