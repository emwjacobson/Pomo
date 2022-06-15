idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES led_manager nvs_flash spiffs wifi_manager)

spiffs_create_partition_image(www ${CMAKE_CURRENT_SOURCE_DIR}/../www FLASH_IN_PROJECT)
