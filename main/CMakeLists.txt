idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES led_manager nvs_flash spiffs wifi_manager)
